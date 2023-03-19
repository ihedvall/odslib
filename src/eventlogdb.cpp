/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <chrono>
#include <util/logstream.h>
#include "ods/databaseguard.h"
#include "eventlogdb.h"

using namespace util::log;
using namespace std::chrono_literals;

namespace ods::detail {
EventLogDb::EventLogDb()
: IEnvironment(EnvironmentType::kTypeEventLogDb) {
  Name("EventLogDb");
  Description("System log application that mainly is used for events.");
}

EventLogDb::~EventLogDb() {
  EventLogDb::Stop();
}

void EventLogDb::DbFileName(const std::string &db_file) {
  db_file_ = db_file;
  database_.FileName(db_file);
}

bool EventLogDb::IsOk() const {
  return is_ok_;
}

bool EventLogDb::Init() {
  is_ok_ = false;
  if (db_file_.empty()) {
    LOG_ERROR() << "The database file name has not been set.";
    return false;
  }

  if (Name().empty()) {
    LOG_ERROR() << "The name has not been set.";
    return false;
  }

  // Check if we need to create the database
  bool need_create_db = false;
  try {
    std::filesystem::path db_path(db_file_);
    need_create_db = !std::filesystem::exists(db_path);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Path error in database path. Error: " << err.what()
                << ", Path: " << db_file_;
    return false;
  }

  if (need_create_db) {
    const auto create = CreateDb();
    if (!create) {
      LOG_ERROR() << "Failed to create the cache database. Path: " << db_file_;
      return false;
    } else {
      LOG_DEBUG() << "Created a database. Database: " << db_file_;
    }
  }

  // No actual initialize the environment
  const auto init = InitDb();
  if (!init) {
    LOG_ERROR() << "Fail to initialize the environment. Environment: " << Name();
    return false;
  } else {
    LOG_DEBUG() << "Read in model from the database. Database: " << DbFileName();
  }

  is_ok_ = true;
  return true;
}

bool EventLogDb::IsStarted() const {
  return worker_thread_.joinable() && !stop_thread_;
}

void EventLogDb::Start() {
  if (IsStarted()) {
    LOG_DEBUG() << "Start called on started worker thread.";
    return;
  }
  if (!IsOk()) {
    LOG_ERROR() << "Init Failed does not start the worker thread. ";
    return;
  }

  for (auto& input : input_list_) {
    if (input) {
      input->Start();
    }
  }

  stop_thread_ = false;
  worker_thread_ = std::thread(&EventLogDb::WorkerThread, this);

}

void EventLogDb::Stop() {
  stop_thread_ = true;
  worker_condition_.notify_one();

  LOG_DEBUG() << "Worker thread request to stop. Environment: " << Name();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }

  for (auto& input : input_list_) {
    if (input) {
      input->Stop();
    }
  }
  LOG_DEBUG() << "Worker thread stopped. Environment: " << Name();
}

void EventLogDb::WorkerThread() {
  is_ok_ = true;
  LOG_DEBUG() << "Worker thread started. Environment: " << Name();

  while (!stop_thread_) {
    std::unique_lock lock(worker_lock_);
    worker_condition_.wait_for(lock, 1s,  [&] {
      return stop_thread_.load();
    });

    DoAllInputMessages();
    DoTrimDatabase();
  }
  LOG_DEBUG() << "Worker thread ready. Environment: " << Name();
  is_ok_ = true;
}

void EventLogDb::AddInput(std::unique_ptr<util::syslog::ISyslogServer> &input) {
  input_list_.push_back(std::move(input));
}

void EventLogDb::DeleteInput(const std::string &input_name) {
  auto find = std::ranges::find_if(input_list_, [&] (const auto& input) {
    return util::string::IEquals(input_name, input->Name());
  });
  if (find != input_list_.end()) {
    input_list_.erase(find);
  }
}

void EventLogDb::AddMessage(const util::syslog::SyslogMessage &msg) {
  const auto* syslog_table = model_.GetTableByName("Syslog");
  if (syslog_table == nullptr) {
    LOG_ERROR() << "Syslog table is missing in database. Environment: " << Name();
    return;
  }
  IItem log_row(syslog_table->ApplicationId());
  log_row.AppendAttribute({"Message", "name",  msg.Message()});
  log_row.AppendAttribute({"LogTime", "date", msg.Timestamp()});
  log_row.AppendAttribute({"Severity", "", static_cast<long>(msg.Severity())});
  log_row.AppendAttribute({"Facility", "", static_cast<long>(msg.Facility())});
  log_row.AppendAttribute({"Hostname", "", AddUniqueName({"Hostname",msg.Hostname()})});
  log_row.AppendAttribute({"Application", "", AddUniqueName({"Application", msg.ApplicationName()})});
  log_row.AppendAttribute({"ProcessID", "", msg.ProcessId()});
  log_row.AppendAttribute({"MessageID", "", msg.MessageId()});

  try {
    Database().Insert(*syslog_table, log_row, SqlFilter());
  } catch (const std::exception& err) {
    LOG_ERROR() << "Syslog insert failed. Error: " << err.what();
    return;
  }

  const auto log_index = log_row.ItemId();

  const auto& sd_data_list = msg.DataList();
  for (const auto& sd_data : sd_data_list) {
    const auto& sd_id = sd_data.Identity();
    if (sd_id.empty()) {
      continue;
    }
    const auto& sd_stem = sd_data.IdentityStem();
    const auto& sd_enterprise = sd_data.EnterpriseId();

    const auto enterprise_id = sd_enterprise.empty() ? 0 : AddUniqueName({"Enterprise", sd_enterprise});

    IItem id_item("SdIdent",sd_id);
    id_item.AppendAttribute({"Stem", "", sd_stem});
    id_item.AppendAttribute({"Enterprise", "", enterprise_id});

    const auto ident_index = AddUniqueName(id_item);
    if (ident_index <= 0) {
      continue;
    }

    const auto& parameter_list = sd_data.Parameters();
    for (const auto& parameter : parameter_list) {
      const auto& parameter_name = parameter.first;
      const auto* parameter_table = model_.GetTableByName("SdName");
      const auto* col_name = parameter_table == nullptr ? nullptr : parameter_table->GetColumnByBaseName("name");
      const auto* col_parent = parameter_table == nullptr ? nullptr : parameter_table->GetColumnByBaseName("parent");
      if (parameter_name.empty() || parameter_table == nullptr
          || col_name == nullptr || col_parent == nullptr) {
        continue;
      }

      SqlFilter parameter_filter;
      parameter_filter.AddWhere(*col_parent, SqlCondition::Equal, ident_index );
      parameter_filter.AddWhere(*col_name, SqlCondition::EqualIgnoreCase, parameter_name );

      IItem name_row(parameter_table->ApplicationId());
      name_row.AppendAttribute({"Name", "name", parameter_name});
      name_row.AppendAttribute({"DataType", "", static_cast<long>(DataType::DtString)});
      name_row.AppendAttribute({"Parent", "parent", ident_index});

      try {
        Database().Insert(*parameter_table, name_row, parameter_filter);
      } catch( const std::exception& err) {
        LOG_ERROR() << "Parameter name insert failed. Error: " << err.what();
        continue;
      }
      const auto name_index = name_row.ItemId();
      if (name_index <= 0) {
        continue;
      }

      const auto& parameter_value = parameter.second;
      const auto* sd_data_table = model_.GetTableByName("SdData");
      if (sd_data_table == nullptr) {
        continue;
      }

      IItem data_row(sd_data_table->ApplicationId());
      data_row.AppendAttribute({"Value", "name", parameter_value});
      data_row.AppendAttribute({"Parent", "parent", log_index});
      data_row.AppendAttribute({"SdName", "", name_index});

      try {
        Database().Insert(*sd_data_table, data_row, SqlFilter());
      } catch( const std::exception& err) {
        LOG_ERROR() << "Insert of parameter data value failed. Error: " << err.what();
      }
    }
  }
}

void EventLogDb::DoAllInputMessages() {
  bool open_once = false;
  for (auto& input : input_list_) {
    if (!input || input->NofMessages() == 0) {
      continue;
    }
    DatabaseGuard db_lock(Database()); // Open and close for each input with messages
    for (auto msg = input->GetMsg(false); db_lock.IsOk() && msg.has_value(); msg = input->GetMsg(false)) {
      AddMessage(msg.value());
    }
  }
}

size_t EventLogDb::GetNofMessages() {
  const auto* table = model_.GetTableByName("Syslog");
  if (table == nullptr) {
    return 0;
  }

  DatabaseGuard db_lock(Database()); // Open and close for each input with messages
  try {
    return Database().Count(*table,SqlFilter());
  } catch (const std::exception& err) {
    LOG_ERROR() << "Count messages failed. Error: " << err.what();
  }
  return 0;
}

void EventLogDb::DoTrimDatabase() {
  if (nof_messages_ < max_nof_messages_) {
    return;
  }
  const auto* table = model_.GetTableByName("Syslog");
  const auto* column_stored = table != nullptr ? table->GetColumnByBaseName("version_date"): nullptr;

  if (table == nullptr || column_stored == nullptr) {
    return;
  }
  { // Must close the database before Vacuum() call
    DatabaseGuard db_lock(Database()); // Open and close for each input with messages
    size_t delete_rows = nof_messages_ - (max_nof_messages_ - 1'000);
    SqlFilter filter;
    filter.AddOrder(*column_stored, SqlCondition::OrderByNone);
    filter.AddLimit(SqlCondition::LimitNofRows, static_cast<int64_t>(delete_rows));
    try {
      Database().Delete(*table, filter);
    } catch (const std::exception &err) {
      LOG_ERROR() << "Deleting messages failed. Error: " << err.what();
    }
  }
  try {
    Database().Vacuum();
  } catch (const std::exception& err) {
    LOG_ERROR() << "Vacumm of database failed. Error: " << err.what();
  }
}

} // end namespace ods