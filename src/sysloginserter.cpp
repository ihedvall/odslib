/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "sysloginserter.h"
#include <boost/program_options.hpp>
#include <vector>
#include <util/syslogmessage.h>
#include <util/stringutil.h>
#include <util/stringutil.h>
#include <workflow/iworkflow.h>
#include "ods/odsfactory.h"
#include "ods/databaseguard.h"
#include "ods/sqlfilter.h"

#include "template_names.icc"

using namespace workflow;
using namespace boost::program_options;
using namespace util::syslog;
using namespace util::string;
using namespace util::time;

using SyslogList = std::vector<SyslogMessage>;

namespace ods {

SyslogInserter::SyslogInserter() {
  Name(kSyslogInserter.data());
  Template(kSyslogInserter.data());
  Description("Insert syslog messages into a database");
}

SyslogInserter::SyslogInserter(const IRunner& source)
    : IRunner(source) {
  Template(kSyslogInserter.data());
  ParseArguments();
}

void SyslogInserter::ParseArguments() {
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

    const auto arg_list = split_winmain(Arguments());
    basic_command_line_parser parser(arg_list);
    parser.options(desc);
    const auto opt = parser.run();
    variables_map var_list;
    store(opt,var_list);
    notify(var_list);
    IsOk(true);
  } catch( const std::exception& err) {
    std::ostringstream msg;
    msg << "Initialization error. Error: " << err.what();
    LastError(msg.str());
    IsOk(false);
  }
}

void SyslogInserter::Init() {
  IRunner::Init();
  ParseArguments();
  if (IEquals(db_type_, "Postgres")) {
    database_ = OdsFactory::CreateDatabase(DbType::TypePostgres);
  } else if (IEquals(db_type_, "SQLite")) {
    database_ = OdsFactory::CreateDatabase(DbType::TypeSqlite);
  }
  try {
    if (database_) {
      database_->ConnectionInfo(connection_string_);
      DatabaseGuard db_lock(*database_);
      const auto read = database_->ReadModel(model_);
      IsOk(read);
    } else {
      IsOk(false);
    }
  } catch (const std::exception& err) {
    IsOk(false);
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
    IsOk(false);
    return;
  }
  if (syslog_list->empty()) {
    return;
  }
  DatabaseGuard db_lock(*database_);
  if (!db_lock.IsOk()) {
    LastError("No database connection.");
    IsOk(false);
    return;
  }
  try {
    for ( const auto& msg : *syslog_list) {
      InsertMessage(msg);
    }
  } catch( const std::exception& err) {
    db_lock.Rollback();
  }
}

void SyslogInserter::Exit() {
  IRunner::Exit();
  database_.reset();
}

void SyslogInserter::InsertMessage(const SyslogMessage &msg) {

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

}

int64_t SyslogInserter::InsertHost(const std::string &hostname) {
  if (hostname.empty()) {
    return 0;
  }
  if (const auto itr = host_cache_.find(hostname); itr != host_cache_.cend()) {
    return itr->second;
  }
  auto* table = model_.GetTableByName("Hostname");
  auto* column_name = table != nullptr ? table->GetColumnByBaseName("name")
                                       : nullptr;
  if (table == nullptr ||  column_name == nullptr) {
    return 0;
  }

  SqlFilter filter;
  filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase, hostname);

  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", hostname);
  row.AppendAttribute(*table, false, "DisplayName", hostname);
  database_->Insert(*table,row,filter);

  return row.ItemId();
}

int64_t SyslogInserter::InsertApplication(const std::string &app_name) {
  if (app_name.empty()) {
    return 0;
  }
  if (const auto itr = app_cache_.find(app_name); itr != app_cache_.cend()) {
    return itr->second;
  }
  auto* table = model_.GetTableByName("Application");
  auto* column_name = table != nullptr ? table->GetColumnByBaseName("name")
                                       : nullptr;
  if (table == nullptr ||  column_name == nullptr) {
    return 0;
  }

  SqlFilter filter;
  filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase, app_name);

  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", app_name);
  row.AppendAttribute(*table, false, "DisplayName", app_name);
  database_->Insert(*table,row,filter);
  return row.ItemId();
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

    IItem key_row(key_table->ApplicationId());
    key_row.AppendAttribute(*key_table, true, "name", key);
    key_row.AppendAttribute(*key_table, true, "parent", identity_idx);
    database_->Insert(*key_table, key_row, filter);
    const auto key_idx = key_row.ItemId();
    if (key_idx == 0) {
      continue;
    }

    IItem value_row(value_table->ApplicationId());
    value_row.AppendAttribute(*value_table, true, "name",value);
    value_row.AppendAttribute(*value_table, true, "parent", msg_idx);
    value_row.AppendAttribute(*value_table, false, "SdName", key_idx);
    database_->Insert(*key_table, key_row, SqlFilter());
  }
}

int64_t SyslogInserter::InsertIdentity(const StructuredData &data){
  const auto& identity = data.Identity();
   if (identity.empty()) {
    return 0;
  }

  if (const auto itr = identity_cache_.find(identity);
      itr != app_cache_.cend()) {
    return itr->second;
  }
  auto* table = model_.GetTableByName("SdIdent");
  auto* column_name = table != nullptr ? table->GetColumnByBaseName("name")
                                       : nullptr;
  if (table == nullptr ||  column_name == nullptr) {
    return 0;
  }

  SqlFilter filter;
  filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase, identity);

  IItem row(table->ApplicationId());
  row.AppendAttribute(*table, true, "name", identity);
  row.AppendAttribute(*table, false, "Stem", data.IdentityStem());
  row.AppendAttribute(*table, false, "Enterprise", data.EnterpriseId());
  database_->Insert(*table,row,filter);
  return row.ItemId();
}

} // namespace ods