/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "syslogservice.h"
#include <algorithm>
#include<ranges>

#include "syslogrpcserver.h"
#include <util/syslogmessage.h>
#include <util/logstream.h>
#include "ods/databaseguard.h"

using namespace ::syslog;
using namespace ::grpc;
using namespace ::util::log;

namespace {

void NsToProtoTimestamp(uint64_t ns1970,
                        google::protobuf::Timestamp* timestamp) {
  if (timestamp != nullptr) {
    timestamp->set_seconds(static_cast<int64_t>(ns1970 / 1'000'000'000));
    timestamp->set_nanos(static_cast<int32_t>(ns1970 % 1'000'000'000));
  }
}

uint64_t ProtoTimestampToNs(const google::protobuf::Timestamp& timestamp) {
  uint64_t ns1970 = 0;
  ns1970 = timestamp.seconds();
  ns1970 *= 1'000'000'000;
  ns1970 += timestamp.nanos();
  return ns1970;
}

util::syslog::SyslogMessage ProtobufMessageToSyslogMessage(
    const SyslogMessage& msg_pb) {
  util::syslog::SyslogMessage msg;
  msg.Index(msg_pb.identity());
  msg.Severity(static_cast<util::syslog::SyslogSeverity>(msg_pb.severity()));
  msg.Facility(static_cast<util::syslog::SyslogFacility>(msg_pb.facility()));
  msg.Timestamp(ProtoTimestampToNs(msg_pb.timestamp()));
  msg.Hostname(msg_pb.hostname());
  msg.ApplicationName(msg_pb.application_name());
  msg.ProcessId(msg_pb.process_id());
  msg.MessageId(msg_pb.message_id());
  msg.Message(msg_pb.text());
  // This function cannot handle data values as the protobuf
  // data value reference a database index
  return msg;
}

} // end namespace

namespace ods {
SyslogService::SyslogService(SyslogRpcServer &server)
    : server_(server)
{
}

Status SyslogService::GetLastEvent(ServerContext *context,
                            const google::protobuf::Empty *request,
                            EventMessage *response) {
  if (response == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Response argument is null");
    return invalid_argument;
  }

  // Fetch the syslog message from the inserter task.
  const auto message = server_.LastMessage();

  // Fill in the response properties.
  response->set_identity(message.Index());
  response->set_severity(static_cast<Severity>(message.Severity()));
  NsToProtoTimestamp(message.Timestamp(), response->mutable_timestamp());
  response->set_text(message.Message());

  return Status::OK;
}

Status SyslogService::GetCount(ServerContext *context,
                               const SyslogFilter *request,
                               SyslogCount *response) {
  if (request == nullptr | response == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Response or request argument is null");
    return invalid_argument;
  }
  auto sql_filter = MakeFilter(*request);
  const auto& model = server_.GetModel();
  auto* table = model.GetTableByName("Syslog");
  auto* database = server_.GetDatabase();
  if (database == nullptr || table == nullptr) {
    Status internal_error(StatusCode::INTERNAL,
                            "Missing database");
    return internal_error;
  }

  DatabaseGuard db_lock(*database);

  try {
    if (!db_lock.IsOk()) {
      throw std::runtime_error("Failed to connect to the database.");
    }
    const auto count = database->Count(*table, sql_filter);
    response->set_count(count);
  } catch (const std::exception& err) {
    db_lock.Rollback();
    Status internal_error(StatusCode::INTERNAL,
                         err.what());
    return internal_error;
  }

  return Status::OK;
}

Status SyslogService::GetEvent(ServerContext *context,
                        const SyslogFilter *request,
                        ServerWriter<EventMessage> *writer) {
  if (request == nullptr | writer == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Response or writer argument is null");
    return invalid_argument;
  }

  // Create an SqlFilter object out of the request.
  auto filter = MakeFilter(*request);

  // Connect to database and fetch the events
  const auto& model = server_.GetModel();
  auto* table = model.GetTableByName("Syslog");
  auto* database = server_.GetDatabase();
  if (database == nullptr || table == nullptr) {
    Status internal_error(StatusCode::INTERNAL,
                          "Missing database");
    return internal_error;
  }

  // Connect to the database and fetch items.
  // With event message, I only need the syslog table columns
  DatabaseGuard db_lock(*database);

  try {
    if (!db_lock.IsOk()) {
      throw std::runtime_error("Failed to connect to the database.");
    }
    database->FetchItems(*table, filter, [&] (IItem& item) {
      EventMessage msg;
      msg.set_identity(item.ItemId());
      msg.set_severity(static_cast<Severity>(item.Value<uint16_t>("Severity")));
      msg.timestamp();
      const auto ns1970 = item.Value<uint64_t>("LogTime");
      NsToProtoTimestamp(ns1970, msg.mutable_timestamp());
      msg.set_text(item.Value<std::string>("Message"));
      writer->Write(msg);
    });

  } catch (const std::exception& err) {
    db_lock.Rollback();
    Status internal_error(StatusCode::INTERNAL,
                          err.what());
    return internal_error;
  }
  return Status::OK;
}

Status SyslogService::GetSyslog(
    ServerContext *context, const SyslogFilter *request,
    ServerWriter<SyslogMessage> *writer) {
  if (request == nullptr | writer == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Response or writer argument is null");
    return invalid_argument;
  }

  // Create an SqlFilter object out of the request.
  auto filter = MakeFilter(*request);

  // Connect to database and fetch the events
  const auto& model = server_.GetModel();
  auto* table = model.GetTableByName("Syslog");

  auto* database = server_.GetDatabase();
  if (database == nullptr || table == nullptr) {
    Status internal_error(StatusCode::INTERNAL,
                          "Missing database");
    return internal_error;
  }

  // Connect to the database and fetch items.
  // With syslog messages, we need names from other table, so fetch them first
  // to speed up the responses.
  DatabaseGuard db_lock(*database);

  try {
    if (!db_lock.IsOk()) {
      throw std::runtime_error("Failed to connect to the database.");
    }
    IdNameMap host_list; // Sorted list idx->names
    IdNameMap app_list;
    IdNameMap unit_list;
    ItemList data_list;
    ItemList data_name_list;

    // We cannot use the name column as the user want's the display name
    // column.
    if ( auto* host_table = model.GetTableByName("Hostname");
         host_table != nullptr) {
      database->FetchItems(*host_table, {}, [&] (IItem& item) {
        const auto idx = item.ItemId();
        const auto name = item.BaseValue<std::string>("name");
        const auto display_name = item.Value<std::string>("DisplayName");
        host_list.emplace(idx, display_name.empty() ? name : display_name);
      });
    }

    if ( auto* app_table = model.GetTableByName("Application");
        app_table != nullptr) {
      database->FetchItems(*app_table, {}, [&] (IItem& item) {
        const auto idx = item.ItemId();
        const auto name = item.BaseValue<std::string>("name");
        const auto display_name = item.Value<std::string>("DisplayName");
        app_list.emplace(idx, display_name.empty() ? name : display_name);
      });
    }

    if ( auto* unit_table = model.GetTableByName("Unit");
        unit_table != nullptr) {
      database->FetchNameMap(*unit_table, unit_list, {});
    }

    if ( auto* data_table = model.GetTableByName("SdData");
        data_table != nullptr) {
      if (const auto* parent_column = data_table->GetColumnByBaseName("parent");
          parent_column != nullptr) {
        SqlFilter data_filter;
        data_filter.AddWhereSelect(*parent_column, SqlCondition::In,
                              *table, filter);
        database->FetchItemList(*data_table, data_list,data_filter);
      }
    }

    if ( auto* data_name_table = model.GetTableByName("SdName");
        data_name_table != nullptr) {
      database->FetchItemList(*data_name_table, data_name_list, {});
    }

    database->FetchItems(*table, filter, [&] (IItem& item) {
      SyslogMessage msg;
      msg.set_identity(item.ItemId());
      msg.set_severity(static_cast<Severity>(item.Value<uint16_t>("Severity")));
      msg.set_facility(item.Value<uint32_t>("Facility"));
      msg.timestamp();
      const auto ns1970 = item.Value<uint64_t>("LogTime");
      NsToProtoTimestamp(ns1970, msg.mutable_timestamp());
      msg.set_text(item.Value<std::string>("Message"));
      if (const auto find_host =
              host_list.find( item.Value<int64_t>("Hostname"));
          find_host != host_list.cend()) {
         const auto& [idx, name] = *find_host;
         msg.set_hostname(name);
      }
      if (const auto find_app =
              app_list.find( item.Value<int64_t>("Application"));
          find_app != app_list.cend()) {
         const auto& [idx, name] = *find_app;
         msg.set_application_name(name);
      }
      msg.set_process_id(item.Value<std::string>("ProcessID"));
      msg.set_message_id(item.Value<std::string>("MessageID"));

      for (const auto& data : data_list) {
        if (data->BaseValue<int64_t>("parent") != item.ItemId()) {
          continue;
        }
        if (auto* data_msg = msg.add_data_values();
            data_msg != nullptr) {
          const auto data_name_idx = data->Value<int64_t>("SdName");
          data_msg->set_identity(data_name_idx);
          data_msg->set_value(data->BaseValue<std::string>("name"));
          const auto data_name_find = std::ranges::find_if(data_name_list,
                                       [&] (const auto& item) {
            return item && item->ItemId() == data_name_idx; });
          if (data_name_find != data_name_list.cend()) {
            const auto& data_name = *data_name_find;
            const auto name = data_name->BaseValue<std::string>("name");
            const auto display_name =
                data_name->Value<std::string>("DisplayName");
            data_msg->set_name(display_name.empty() ? name : display_name);
            if (const auto find_unit = unit_list.find(
                    data_name->Value<int64_t>("unit"));
                find_unit != unit_list.cend()) {
              data_msg->set_unit(find_unit->second);
            }
          }
        }
      }
      writer->Write(msg);
    });

  } catch (const std::exception& err) {
    db_lock.Rollback();
    Status internal_error(StatusCode::INTERNAL,
                          err.what());
    return internal_error;
  }
  return Status::OK;
}

Status SyslogService::GetDataDefinitions(
    ServerContext *context, const google::protobuf::Empty *request,
    ServerWriter<SyslogDataDefinition> *writer) {
  if (writer == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Writer argument is null");
    return invalid_argument;
  }
  // Connect to database and fetch the events
  const auto& model = server_.GetModel();
  auto* table = model.GetTableByName("SdName");
  auto* database = server_.GetDatabase();
  if (database == nullptr || table == nullptr) {
    Status internal_error(StatusCode::INTERNAL,
                          "Missing database or missing SdName table");
    return internal_error;
  }

  // Connect to the database and fetch items.
  // With syslog messages, we need names from other table, so fetch them first
  // to speed up the responses.
  DatabaseGuard db_lock(*database);

  try {
    if (!db_lock.IsOk()) {
      throw std::runtime_error("Failed to connect to the database.");
    }
    IdNameMap unit_list;
    if ( auto* unit_table = model.GetTableByName("Unit");
        unit_table != nullptr) {
      database->FetchNameMap(*unit_table, unit_list, {});
    }

    SqlFilter empty_filter {};
    database->FetchItems(*table, empty_filter, [&] (IItem& item) {
      SyslogDataDefinition msg;
      msg.set_identity(item.ItemId());
      msg.set_name(item.BaseValue<std::string>("name"));
      msg.set_display_name(item.Value<std::string>("DisplayName"));
      msg.set_description(item.BaseValue<std::string>("description"));

      std::string unit;
      const auto idx = item.Value<int64_t>("Unit");
      if (idx != 0) {
        const auto itr = unit_list.find(idx);
        if (itr != unit_list.cend()) {
          unit = itr->second;
        }
      }
      msg.set_unit(unit);
      writer->Write(msg);
    });

  } catch (const std::exception& err) {
    db_lock.Rollback();
    Status internal_error(StatusCode::INTERNAL, err.what());
    return internal_error;
  }
  return Status::OK;
}

SqlFilter SyslogService::MakeFilter(const SyslogFilter &syslog_filter) const {
  const auto& model = server_.GetModel();

  SqlFilter filter;
  const auto* table = model.GetTableByName("Syslog");
  if (table == nullptr) {
    ::LOG_ERROR() << "Syslog table not found";
    return filter;
  }

  if (const auto* severity_column = table->GetColumnByName("Severity");
      severity_column != nullptr && syslog_filter.has_level()) {
    filter.AddWhere(*severity_column, SqlCondition::GreaterEQ,
                    static_cast<int>(syslog_filter.level()));
  }
  if (const auto* facility_column = table->GetColumnByName("Facility");
      facility_column != nullptr && syslog_filter.has_facility()) {
    filter.AddWhere(*facility_column, SqlCondition::Equal,
                    static_cast<int>(syslog_filter.facility()));
  }
  // Todo: Add message filter.
  // Todo: Add from time and to time.
  // Todo: Add data filter.
  if (const auto* id_column = table->GetColumnByBaseName("id");
      id_column != nullptr && syslog_filter.has_from_id()) {
    filter.AddWhere(*id_column, SqlCondition::GreaterEQ,
                    syslog_filter.from_id());
  }

  if (syslog_filter.offset() > 0) {
    filter.AddLimit(SqlCondition::LimitOffset, syslog_filter.offset());
  }
  if (syslog_filter.has_count() && syslog_filter.count() > 0) {
    filter.AddLimit(SqlCondition::LimitNofRows, syslog_filter.count());
  }
  return filter;
}

grpc::Status SyslogService::AddNewMessage(grpc::ServerContext *context,
                                          const syslog::SyslogMessage *request,
                                          google::protobuf::Empty *response) {
  if (request == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Request argument is null");
    return invalid_argument;
  }
  auto* database = server_.GetDatabase();
  if (database == nullptr) {
    Status internal_error(StatusCode::INTERNAL,
                          "Missing database");
    return internal_error;
  }

  // Convert to a C++ syslog object, so it works with the inserter task
  auto msg = ProtobufMessageToSyslogMessage(*request);

  // Create a syslog inserter task and add a syslog message
  {
    SyslogInserter inserter(*database);
    inserter.Init();
    inserter.AddOneMessage(msg);
    inserter.Exit();
  }
  return Status::OK;

}

} // namespace ods