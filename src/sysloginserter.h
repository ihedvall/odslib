/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#pragma once

#include <workflow/irunner.h>
#include <map>
#include "ods/idatabase.h"
#include <util/syslogmessage.h>

#include <util/stringutil.h>
namespace ods {

class SyslogInserter : public workflow::IRunner {
public:
  SyslogInserter();
  explicit SyslogInserter(const IRunner& source);
  void Init() override;
  void Tick() override;
  void Exit() override;

private:
  using CacheList = std::map<std::string, int64_t, util::string::IgnoreCase>;
  size_t data_slot_ = 0;
  std::string db_type_ = "SQLite";
  std::string connection_string_; ///< File name or connection string
  std::unique_ptr<IDatabase> database_;
  IModel model_;
  CacheList host_cache_;
  CacheList app_cache_;
  CacheList identity_cache_;
  void ParseArguments();
  void InsertMessage(const util::syslog::SyslogMessage& msg);
  int64_t InsertHost(const std::string& hostname);
  int64_t InsertApplication(const std::string& app_name);
  void InsertData(const util::syslog::StructuredData& data, int64_t msg_idx);
  int64_t InsertIdentity(const util::syslog::StructuredData& data);
};

} // namespace ods
