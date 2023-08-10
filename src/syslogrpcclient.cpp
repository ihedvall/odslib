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

namespace {
uint64_t TimestampToNs(const ::google::protobuf::Timestamp& timestamp) {
  uint64_t ns1970 = timestamp.seconds();
  ns1970 *= 1'000'000'000;
  ns1970 += timestamp.nanos();
  return ns1970;
}

void NsToTimestamp(uint64_t ns1970, ::google::protobuf::Timestamp& timestamp) {
  timestamp.set_seconds(static_cast<int64_t>(ns1970 / 1'000'000'000));
  timestamp.set_nanos(static_cast<int32_t>(ns1970 % 1'000'000'000));
}

void EventToSyslog(const syslog::SyslogMessage& event,
              util::syslog::SyslogMessage& syslog) {
  syslog.Index(event.identity());
  syslog.Severity(static_cast<util::syslog::SyslogSeverity>(event.severity()));
  syslog.Facility(static_cast<util::syslog::SyslogFacility>(event.facility()));
  syslog.Timestamp(TimestampToNs(event.timestamp()));
  syslog.Message(event.text());
  syslog.Hostname(event.hostname());
  syslog.ApplicationName(event.application_name());
  syslog.ProcessId(event.process_id());
  syslog.MessageId(event.message_id());
  auto data_list = event.data_values();
  for (const auto &data_value : data_list) {
    const auto sd_name_idx = data_value.identity();
    const auto sd_name = data_value.name();
    const auto sd_value = data_value.value();
    syslog.AddStructuredData(std::to_string(sd_name_idx));
    syslog.AppendParameter(sd_name, sd_value);
  }
}

void SyslogToEvent(const util::syslog::SyslogMessage& syslog,
                   syslog::SyslogMessage& event) {
  event.set_identity(syslog.Index());

  event.set_severity(static_cast<syslog::Severity>(syslog.Severity()));
  event.set_facility(static_cast<uint32_t>(syslog.Facility()));
  if (auto* time = event.mutable_timestamp(); time != nullptr ) {
    NsToTimestamp(syslog.Timestamp(), *time);
  }
  event.set_text(syslog.Message());
  event.set_hostname(syslog.Hostname());
  event.set_application_name(syslog.ApplicationName());
  event.set_process_id(syslog.ProcessId());
  event.set_message_id(syslog.MessageId());
}

} // end namespace

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
    msg.Timestamp(TimestampToNs(event.timestamp()));
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

void SyslogRpcClient::GetEventList(
    std::vector<util::syslog::SyslogMessage>& event_list) {

  try {
    ClientContext context;
    auto reader(stub_->GetEvent(&context, filter_));
    for (EventMessage event; reader->Read(&event); event.Clear()) {
      util::syslog::SyslogMessage syslog;
      syslog.Index(event.identity());
      syslog.Severity(static_cast<util::syslog::SyslogSeverity>(
          event.severity()));
      syslog.Timestamp(TimestampToNs(event.timestamp()));
      syslog.Message(event.text());
      event_list.emplace_back(syslog);
    }

    const auto status = reader->Finish();
    if (!status.ok()) {
      throw std::runtime_error(status.error_message());
    }

    operable_ = true;
  } catch (const std::exception& err) {
    if (operable_) {
      LOG_ERROR() << "Get event request failed. Error: " << err.what();
    }
    operable_ = false;
  }
}

void SyslogRpcClient::GetSyslogList(
    std::vector<util::syslog::SyslogMessage>& syslog_list) {

    try {
      ClientContext context;
      auto reader(stub_->GetSyslog(&context, filter_));
      for (SyslogMessage event; reader->Read(&event); event.Clear()) {
        util::syslog::SyslogMessage syslog;
        syslog.Index(event.identity());
        syslog.Severity(static_cast<util::syslog::SyslogSeverity>(
            event.severity()));
        syslog.Facility(static_cast<util::syslog::SyslogFacility>(
            event.facility()));
        syslog.Timestamp(TimestampToNs(event.timestamp()));
        syslog.Message(event.text());
        syslog.Hostname(event.hostname());
        syslog.ApplicationName(event.application_name());
        syslog.ProcessId(event.process_id());
        syslog.MessageId(event.message_id());
        auto data_list = event.data_values();
        for (const auto& data_value : data_list) {
          const auto sd_name_idx = data_value.identity();
          const auto sd_name = data_value.name();
          const auto sd_value = data_value.value();
          syslog.AddStructuredData(std::to_string(sd_name_idx));
          syslog.AppendParameter(sd_name,sd_value);
        }
        syslog_list.emplace_back(syslog);
      }

      const auto status = reader->Finish();
      if (!status.ok()) {
        throw std::runtime_error(status.error_message());
      }

      operable_ = true;
    } catch (const std::exception& err) {
      if (operable_) {
        LOG_ERROR() << "Get event request failed. Error: " << err.what();
      }
      operable_ = false;
    }

}

void SyslogRpcClient::AddEvent(const util::syslog::SyslogMessage &event) {

    try {
      ClientContext context;
      SyslogMessage request;
     SyslogToEvent(event, request);
      google::protobuf::Empty reply;
      const auto status = stub_->AddNewMessage(&context, request, &reply);
      if (!status.ok()) {
        throw std::runtime_error(status.error_message());
      }
      operable_ = true;
    } catch (const std::exception& err) {
      if (operable_) {
        LOG_ERROR() << "Add event request failed. Error: " << err.what();
      }
      operable_ = false;
    }
}

void SyslogRpcClient::Clear() {filter_.Clear();}
void SyslogRpcClient::Level(util::syslog::SyslogSeverity severity) {
  filter_.set_level(static_cast<Severity>(severity));
}
void SyslogRpcClient::Facility(uint8_t facility) {
  filter_.set_facility(facility);
}

void SyslogRpcClient::TextFilter(const string &wildcard) {
  filter_.set_text_filter(wildcard);
}

void SyslogRpcClient::TimeFrom(uint64_t ns1970) {
  if (auto* time = filter_.mutable_from_time();
      time != nullptr) {
    time->set_seconds(static_cast<int64_t>(ns1970 / 1'000'000'000));
    time->set_nanos(static_cast<int32_t>(ns1970 % 1'000'000'000));
  } else {
    filter_.clear_from_time();
  }
}

void SyslogRpcClient::TimeTo(uint64_t ns1970) {
  if (auto *time = filter_.mutable_to_time(); time != nullptr) {
    time->set_seconds(static_cast<int64_t>(ns1970 / 1'000'000'000));
    time->set_nanos(static_cast<int32_t>(ns1970 % 1'000'000'000));
  } else {
    filter_.clear_to_time();
  }
}


} // namespace ods