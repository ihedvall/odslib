/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#pragma once

#include <workflow/itask.h>
#include <map>
#include <mutex>
#include "ods/imodel.h"
#include "ods/idatabase.h"
#include <util/syslogmessage.h>

#include <util/stringutil.h>
namespace ods {

class SyslogInserter : public workflow::ITask {
public:
  /** \brief Default constructor mainly used for unit tests. */
  SyslogInserter();

  /** \brief Constructor with a runner task definition as input. */
  explicit SyslogInserter(const ITask& source);

  /** \brief Constructor with an existing database. */
  explicit SyslogInserter(const IDatabase& database);

  void Init() override;
  void Tick() override;
  void Exit() override;

  /** \brief Adds one syslog message to the database.
   *
   * Opens the database and adds one message to the database. This function
   * is mainly used for unit test and manually adding small amount of messages.
   * The InsertMessage() is much more effective if adding a lot of messages.
   *
   * @param msg Syslog message
   * @return True if successful insert.
   */
  bool AddOneMessage( util::syslog::SyslogMessage& msg);
  [[nodiscard]] size_t GetNofMessages();
  [[nodiscard]] util::syslog::SyslogMessage LastMessage() const;
private:
  using CacheList = std::map<std::string, int64_t, util::string::IgnoreCase>;
  std::string db_type_ = "SQLite";
  std::string connection_string_; ///< File name or connection string
  std::unique_ptr<IDatabase> database_;
  IModel model_;
  CacheList host_cache_;
  CacheList app_cache_;
  CacheList identity_cache_;

  mutable std::mutex last_message_locker_;
  util::syslog::SyslogMessage last_message_;

  void ParseArguments();
  void InsertMessage( util::syslog::SyslogMessage& msg);
  int64_t InsertHost(const std::string& hostname);
  int64_t InsertApplication(const std::string& app_name);
  void InsertData(const util::syslog::StructuredData& data, int64_t msg_idx);
  int64_t InsertIdentity(const util::syslog::StructuredData& data);
};

} // namespace ods
