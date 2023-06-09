/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "syslogservice.h"
#include "syslogrpcserver.h"
#include <util/syslogmessage.h>
#include <util/logstream.h>
#include "ods/databaseguard.h"

using namespace ::syslog;
using namespace ::util::log;
using namespace ::grpc;

namespace ods {
SyslogService::SyslogService(SyslogRpcServer &server)
    : server_(server)
{
}

SyslogService::~SyslogService() {}

Status SyslogService::GetLastEvent(grpc::ServerContext *context,
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
  auto* time = response->mutable_timestamp();
  if (time != nullptr) {
    time->set_seconds(
        static_cast<int64_t>(message.Timestamp() / 1'000'000'000));
    time->set_nanos(static_cast<int32_t>(message.Timestamp() % 1'000'000'000));
  }
  response->set_text(message.Message());

  return Status::OK;
}

Status SyslogService::GetCount(ServerContext *context,
                               const SyslogFilter *request,
                                     SyslogCount *response) {
  if (request == nullptr | response == nullptr) {
    Status invalid_argument(StatusCode::INVALID_ARGUMENT,
                            "Response argument is null");
    return invalid_argument;
  }
  auto filter = MakeFilter(*request);
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
    const auto count = database->Count(*table, filter);
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
  return Service::GetEvent(context, request, writer);
}

Status SyslogService::GetSyslog(
    ServerContext *context, const SyslogFilter *request,
    ServerWriter<SyslogMessage> *writer) {
  return Service::GetSyslog(context, request, writer);
}

Status SyslogService::GetDataDefinitions(
    ServerContext *context, const google::protobuf::Empty *request,
    ServerWriter<SyslogDataDefinition> *writer) {
  return Service::GetDataDefinitions(context, request, writer);
}

SqlFilter SyslogService::MakeFilter(const SyslogFilter &syslog_filter) const {
  const auto& model = server_.GetModel();

  SqlFilter filter;
  const auto* table = model.GetTableByName("Syslog");
  if (table == nullptr) {
    LOG_ERROR() << "Syslog table not found";
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


} // namespace ods