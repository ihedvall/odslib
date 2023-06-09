/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <util/syslogmessage.h>

#include "proto/syslogservice.pb.h"
#include "proto/syslogservice.grpc.pb.h"

namespace ods {

class SyslogRpcClient {
public:
  SyslogRpcClient() = default;
  SyslogRpcClient(std::string host, uint16_t port);
  virtual ~SyslogRpcClient();

  void Start();
  void Stop();
  [[nodiscard]] bool Operable() const {return operable_;}

  [[nodiscard]] util::syslog::SyslogMessage GetLastEvent();
  [[nodiscard]] size_t GetCount();

  void Clear();
  void Level(util::syslog::SyslogSeverity severity);
  void Facility(uint8_t facility);
  void TextFilter(const std::string& wildcard);

private:
  std::string host_ = "localhost";
  uint16_t port_ = 50600;
  std::unique_ptr<syslog::SyslogService::Stub> stub_;
  bool operable_ = false;
  ::syslog::SyslogFilter filter_; ///< Common filter object for the client
  void MakeSyslogFilter(const util::syslog::SyslogMessage& filter,
                        ::syslog::SyslogFilter& dest);
};

} // namespace ods
