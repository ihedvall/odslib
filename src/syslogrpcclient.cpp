/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "syslogrpcclient.h"
#include <sstream>
#include <utility>
#include <grpcpp/create_channel.h>
#include <util/logstream.h>

using namespace ::syslog;
using namespace ::grpc;
using namespace util::log;
namespace ods {

SyslogRpcClient::SyslogRpcClient(std::string host, uint16_t port)
: host_(std::move(host)),
  port_(port) {

}

SyslogRpcClient::~SyslogRpcClient() {
  Stop();
}

void SyslogRpcClient::Start() {
  try {
    std::stringstream address;
    address << host_ << ":" << port_;
    auto channel = CreateChannel(address.str(), InsecureChannelCredentials());
    stub_ = std::move(SyslogService::NewStub(channel));
    operable_ = true;
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to create the client connection. Error: "
                << err.what();
    operable_ = false;
  }
}

void SyslogRpcClient::Stop() {
  stub_.reset();
  operable_ = false;
}

util::syslog::SyslogMessage SyslogRpcClient::GetLastEvent() {
  util::syslog::SyslogMessage msg;
  try {
    ClientContext context;
    google::protobuf::Empty request;
    EventMessage event;
    auto status = stub_->GetLastEvent(&context, request, &event);
    if (!status.ok()) {
      throw std::runtime_error(status.error_message());
    }
    msg.Index(event.identity());
    msg.Severity(static_cast<util::syslog::SyslogSeverity>(event.identity()));

    auto ns1970 = static_cast<uint64_t>(event.timestamp().seconds());
    ns1970 *= 1'000'000'000;
    ns1970 += event.timestamp().nanos();
    msg.Timestamp(ns1970);

    msg.Message(event.text());
    operable_ = true;
  } catch (const std::exception& err) {
    if (operable_) {
      LOG_ERROR() << "Last event request failed. Error: " << err.what();
    }
    operable_ = false;
  }
  return msg;
}

size_t SyslogRpcClient::GetCount() {
  size_t count = 0;
  try {
    ClientContext context;
    SyslogCount response;
    auto status = stub_->GetCount(&context, filter_, &response);
    if (!status.ok()) {
      throw std::runtime_error(status.error_message());
    }
    count = static_cast<size_t>(response.count());
    operable_ = true;
  } catch (const std::exception& err) {
    if (operable_) {
      LOG_ERROR() << "Last event request failed. Error: " << err.what();
    }
    operable_ = false;
  }
  return count;
}

void SyslogRpcClient::Clear() {filter_.Clear();}
void SyslogRpcClient::Level(util::syslog::SyslogSeverity severity) {
  filter_.set_level(static_cast<Severity>(severity));
}

} // namespace ods