/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <memory>

#include <util/syslogmessage.h>
#include <util/isyslogserver.h>
#include "sqlitedatabase.h"
#include "ods/ienvironment.h"

namespace ods::detail {

class EventLogDb : public IEnvironment {
 public:
  EventLogDb();
  ~EventLogDb() override;
  void DbFileName(const std::string& db_file);

  [[nodiscard]] const std::string& DbFileName() const {
    return db_file_;
  }

  void MaxMessages(size_t max_messages) {
    max_nof_messages_ = max_messages < 2000 ?  2000 : max_messages;
  }

  [[nodiscard]] size_t MaxMessages() const {
    return max_nof_messages_;
  }

  [[nodiscard]] bool IsOk() const override;

  bool Init() override;
  void AddInput(std::unique_ptr<util::syslog::ISyslogServer>& input);
  void DeleteInput(const std::string& input_name);

  [[nodiscard]] bool IsStarted() const override;
  void Start() override;
  void Stop() override;

 private:
  std::string db_file_;     ///< Database file name with full path.
  SqliteDatabase database_; ///< ODS database

  std::atomic<bool> is_ok_ = false;
  std::atomic<bool> stop_thread_ = false;
  std::thread worker_thread_;
  std::mutex  worker_lock_;
  std::condition_variable worker_condition_;

  using InputList = std::vector<std::unique_ptr<util::syslog::ISyslogServer>>;
  InputList input_list_;
  std::atomic<size_t> nof_messages_ = 0;
  size_t max_nof_messages_ = 1'000'000;

  void WorkerThread();
  void AddMessage(const util::syslog::SyslogMessage& msg);
  void DoAllInputMessages();
  void DoTrimDatabase();
  size_t GetNofMessages();

  int64_t AddHostname(const std::string& hostname);
  int64_t AddAppName(const std::string& app_name);
  int64_t AddEnterprise(const std::string& enterprise_id);
};

} // ods
