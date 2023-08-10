/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "sysloginserter.h"
#include <boost/program_options.hpp>
#include <vector>
#include <util/syslogmessage.h>
#include <util/stringutil.h>
#include <util/logstream.h>
#include <workflow/iworkflow.h>
#include "ods/sqlfilter.h"
#include "ods/odsfactory.h"
#include "ods/databaseguard.h"

#include "template_names.icc"

using namespace workflow;
using namespace boost::program_options;
using namespace util::syslog;
using namespace util::string;
using namespace util::time;
using namespace util::log;
using SyslogList = std::vector<SyslogMessage>;

namespace ods {

SyslogInserter::SyslogInserter() {
  Name(kSyslogInserter.data());
  Template(kSyslogInserter.data());
  Description("Insert syslog messages into a database");
  std::ostringstream temp;
  temp << "--slot=" << data_slot_ << " ";
  temp << "--dbtype=" << db_type_ << " ";
  temp << "--connection=\"" << connection_string_ << "\"";
  Arguments(temp.str());
}

SyslogInserter::SyslogInserter(const IRunner& source)
    : IRunner(source) {
  Template(kSyslogInserter.data());
  ParseArguments();
}

SyslogInserter::SyslogInserter(const IDatabase &database)
    : db_type_(database.DatabaseTypeAsString()),
      connection_string_(database.ConnectionInfo())
{
  Name(kSyslogInserter.data());
  Template(kSyslogInserter.data());
  Description("Insert syslog messages into a database");
  std::ostringstream temp;
  temp << "--slot=" << data_slot_ << " ";
  temp << "--dbtype=" << db_type_ << " ";
  temp << "--connection=\"" << connection_string_ << "\"";
  Arguments(temp.str());
}

void SyslogInserter::ParseArguments() {
  std::string arguments = Arguments();
  // If nor arguments are given, the copy the insert syslog task arguments.
  if (const auto* inserter = GetRunnerByTemplateName(kSyslogInserter.data());
      inserter != nullptr) {
    arguments = inserter->Arguments();
  }
  try {
    options_description desc("Available Arguments");
    desc.add_options() ("slot,S",
                       value<size_t>(&data_slot_),
                       "Slot index for data" );
    desc.add_options() ("dbtype,D",
                       value<std::string>(&db_type_),
                       "Database type (default SQLite" );
    desc.add_options() ("connection,C",
                       value<std::string>(&connection_string_),
                       "File name or connection string" );

    const auto arg_list = split_winmain(arguments);
    basic_command_line_parser parser(arg_list);
    parser.options(desc);
    const auto opt = parser.run();
    variables_map var_list;
    store(opt,var_list);
    notify(var_list);
    IsOk(true);
  } catch( const std::exception& err) {
    LastError("Parse argument error");
    IsOk(false);
    LOG_ERROR() << "Parse argument error. Name: " << Name()
                << ", Error: " << err.what();
  }
}

void SyslogInserter::Init() {
  IRunner::Init();
  ParseArguments();
  database_ = OdsFactory::CreateDatabase(
      IDatabase::StringAsDatabaseType(db_type_));

  try {
    if (database_) {
      database_->ConnectionInfo(connection_string_);
      DatabaseGuard db_lock(*database_);
      const auto read = database_->ReadModel(model_);
      IsOk(read);
      if (!read) {
        LOG_ERROR() << "Failed to read the model from the database. Name: "
                    << Name();
      }
    } else {
      IsOk(false);
      LOG_ERROR() << "Missing database. Name: " << Name();
    }
  } catch (const std::exception& err) {
    IsOk(false);
    LOG_ERROR() << "Init error. Name: " << Name()
                << ", Error: " << err.what();
  }
  if (!IsOk()) {
    LastError("Init error");
  }
}

void SyslogInserter::Tick() {
  IRunner::Tick();
  auto* workflow = GetWorkflow();
  auto* syslog_list = workflow != nullptr ?
                        workflow->GetData<SyslogList>(data_slot_) :
                        nullptr;
  if (syslog_list == nullptr) {
    LastError("No syslog list found");
    if (IsOk()) {
      LOG_ERROR() << "Tick error. Name: " << Name()
                  << ", Error: No syslog list found.";
    }
    IsOk(false);
    return;
  }
  if (syslog_list->empty()) {
    // Nothing to do. So no need to open the database.
    return;
  }

  // Open the database and insert all messages in the message queue.
  DatabaseGuard db_lock(*database_);
  if (!db_lock.IsOk()) {
    LastError("No database connection.");
    if (IsOk()) {
      LOG_ERROR() << "Tick database error. Name: " << Name()
                  << ", Error: Database is not OK.";
    }
    IsOk(false);
    return;
  }
  try {
    for ( auto& msg : *syslog_list) {
      InsertMessage(msg);
    }
    IsOk(true);
  } catch( const std::exception& err) {
    db_lock.Rollback();
    if (IsOk()) {
      LOG_ERROR() << "Tick insert error. Name: " << Name()
                  << ", Error: Failed to insert.";
    }
    LastError("Failed to insert into the database");
    IsOk(false);
  }
}

void SyslogInserter::Exit() {
  IRunner::Exit();
  database_.reset();
}

void SyslogInserter::InsertMessage(SyslogMessage &msg) {

  auto* table = model_.GetTableByName("Syslog");
  if (table == nullptr ) {
    return;
  }
  const auto& hostname = msg.Hostname();
  const auto& app_name = msg.ApplicationName();

  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", msg.Message());
  row.AppendAttribute(*table, true, "date", msg.Timestamp());
  row.AppendAttribute(*table, false, "Severity",
                      static_cast<int>(msg.Severity()));
  row.AppendAttribute(*table, false, "Facility",
                      static_cast<int>(msg.Facility()));

  row.AppendAttribute(*table, false, "Hostname",
                      InsertHost(hostname));
  row.AppendAttribute(*table, false, "Application",
                      InsertApplication(app_name));
  row.AppendAttribute(*table, false, "ProcessID",
                      msg.ProcessId());
  row.AppendAttribute(*table, false, "MessageID",
                      msg.MessageId());

  database_->Insert(*table, row, SqlFilter());

  const auto& sd_list = msg.DataList();
  for (const auto& data : sd_list) {
    InsertData(data, row.ItemId());
  }
  msg.Index(row.ItemId());

  std::lock_guard lock(last_message_locker_);
  last_message_ = msg;
}

int64_t SyslogInserter::InsertHost(const std::string &hostname) {
  if (hostname.empty()) {
    return 0;
  }
  if (const auto find = host_cache_.find(hostname); find != host_cache_.cend()) {
    return find->second;
  }

  auto* table = model_.GetTableByName("Hostname");
  auto* column_name = table != nullptr ? table->GetColumnByBaseName("name")
                                       : nullptr;
  if (table == nullptr ||  column_name == nullptr) {
    return 0;
  }

  SqlFilter filter;
  filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase, hostname);
  auto idx = database_->Exists(*table, filter);
  if (idx != 0) {
    host_cache_.emplace(hostname, idx);
    return idx;
  }

  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", hostname);
  row.AppendAttribute(*table, false, "DisplayName", hostname);
  database_->Insert(*table,row,filter);
  idx = row.ItemId();
  if (idx != 0) {
    host_cache_.emplace(hostname, idx);
  }
  return idx;
}

int64_t SyslogInserter::InsertApplication(const std::string &app_name) {
  if (app_name.empty()) {
    return 0;
  }
  if (const auto find = app_cache_.find(app_name); find != app_cache_.cend()) {
    return find->second;
  }
  auto* table = model_.GetTableByName("Application");
  auto* column_name = table != nullptr ? table->GetColumnByBaseName("name")
                                       : nullptr;
  if (table == nullptr ||  column_name == nullptr) {
    return 0;
  }

  SqlFilter filter;
  filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase, app_name);
  auto idx = database_->Exists(*table, filter);
  if (idx != 0) {
    app_cache_.emplace(app_name, idx);
    return idx;
  }

  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", app_name);
  row.AppendAttribute(*table, false, "DisplayName", app_name);
  database_->Insert(*table,row,filter);
  idx = row.ItemId();
  if (idx != 0) {
    app_cache_.emplace(app_name, idx);
  }
  return idx;
}

void SyslogInserter::InsertData(const StructuredData &data, int64_t msg_idx) {
  const auto identity_idx = InsertIdentity(data);
  if (identity_idx == 0) {
    return;
  }

  auto* key_table = model_.GetTableByName("SdName"); // Key table
  auto* key_name = key_table != nullptr ?
                                        key_table->GetColumnByBaseName("name")
                                       : nullptr;
  auto* key_parent = key_table != nullptr ?
                                       key_table->GetColumnByBaseName("parent")
                                       : nullptr;

  auto* value_table = model_.GetTableByName("SdData"); // Value table
  auto* value_name = value_table != nullptr ?
                                        value_table->GetColumnByBaseName("name")
                                        : nullptr;
  auto* value_parent = value_table != nullptr ?
                                        value_table->GetColumnByBaseName("parent")
                                        : nullptr;
  if (key_table == nullptr || key_name == nullptr || key_parent == nullptr ||
    value_table == nullptr || value_name == nullptr ||
    value_parent == nullptr) {
    return;
  }

  // Insert the key value pair
  const auto& parameter_list = data.Parameters();
  for (const auto& parameter : parameter_list) {
    const auto& key = parameter.first;
    const auto& value = parameter.second;
    SqlFilter filter;
    filter.AddWhere(*key_name, SqlCondition::EqualIgnoreCase, key);
    filter.AddWhere(*key_parent, SqlCondition::EqualIgnoreCase, identity_idx);

    auto key_idx = database_->Exists(*key_table, filter);
    if (key_idx == 0) {
      IItem key_row(key_table->ApplicationId());
      key_row.AppendAttribute(*key_table, true, "name", key);
      key_row.AppendAttribute(*key_table, true, "parent", identity_idx);
      database_->Insert(*key_table, key_row, filter);
      key_idx = key_row.ItemId();
    }
    if (key_idx == 0) {
      continue;
    }

    IItem value_row(value_table->ApplicationId());
    value_row.AppendAttribute(*value_table, true, "name",value);
    value_row.AppendAttribute(*value_table, true, "parent", msg_idx);
    value_row.AppendAttribute(*value_table, false, "SdName", key_idx);
    database_->Insert(*key_table, value_row, SqlFilter());
  }
}

int64_t SyslogInserter::InsertIdentity(const StructuredData &data){
  const auto& identity = data.Identity();
   if (identity.empty()) {
    return 0;
  }


  // As we only insert and never delete identities on this table, we
  // can first try the internal cache to see if it is
  if (const auto find = identity_cache_.find(identity);
      find != identity_cache_.cend()) {
    return find->second;
  }

  auto* table = model_.GetTableByName("SdIdent");
  auto* column_name = table != nullptr ? table->GetColumnByBaseName("name")
                                       : nullptr;
  if (table == nullptr ||  column_name == nullptr) {
    return 0;
  }


  SqlFilter filter;
  filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase, identity);
  // Next is to check if the identity already exist in the table
  auto idx = database_->Exists(*table, filter);
  if (idx != 0) {
    identity_cache_.emplace(identity, idx);
    return idx;
  }

  // Not existing now insert a new item
  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", identity);
  row.AppendAttribute(*table, false, "Stem", data.IdentityStem());
  row.AppendAttribute(*table, false, "Enterprise", data.EnterpriseId());
  database_->Insert(*table,row,filter);
  idx = row.ItemId();
  if (idx != 0) {
    identity_cache_.emplace(identity, idx);
  }
  return idx;
}

util::syslog::SyslogMessage SyslogInserter::LastMessage() const {
  std::lock_guard lock(last_message_locker_);
  return last_message_;
}

bool SyslogInserter::AddOneMessage(SyslogMessage &msg) {
  DatabaseGuard db_lock(*database_);
  if (!db_lock.IsOk()) {
    LOG_ERROR() << "Failed to open the database. Name: " << Name();
    return false;
  }

  try {
    InsertMessage(msg);
  } catch( const std::exception& err) {
    LOG_ERROR() << "Failed to insert syslog message. Name: " << Name()
                << ", Error: "  << err.what();
    db_lock.Rollback();
    return false;
  }
  return true;
}

size_t SyslogInserter::GetNofMessages() {
  size_t count = 0;
  DatabaseGuard db_lock(*database_);
  if (!db_lock.IsOk()) {
    LOG_ERROR() << "Failed to open the database. Name: " << Name();
    return 0;
  }
  try {
    // select count(*) from syslog;
    auto* table = model_.GetTableByName("Syslog");
    if (table == nullptr) {
      throw std::runtime_error("No syslog table");
    }
    count = database_->Count(*table, {});
  } catch (const std::exception &err) {
    LOG_ERROR() << "Failed to count syslog message. Name: " << Name()
                << ", Error: "  << err.what();
    db_lock.Rollback();
  }
  return count;
}


} // namespace ods