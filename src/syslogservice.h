/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#pragma once
#include "proto/syslogservice.pb.h"
#include "proto/syslogservice.grpc.pb.h"
#include "ods/sqlfilter.h"
namespace ods {
class SyslogRpcServer;
class SyslogService final : public syslog::SyslogService::Service {
public:
  SyslogService() = delete;
  explicit SyslogService(SyslogRpcServer& server);
  ~SyslogService() override = default;

  grpc::Status GetLastEvent(grpc::ServerContext *context,
                            const google::protobuf::Empty *request,
                            syslog::EventMessage *response) override;

  grpc::Status GetCount(grpc::ServerContext *context,
                        const syslog::SyslogFilter *request,
                        syslog::SyslogCount *response) override;

  grpc::Status  GetEvent(grpc::ServerContext *context,
           const syslog::SyslogFilter *request,
           grpc::ServerWriter<syslog::EventMessage> *writer) override;

  grpc::Status GetSyslog(grpc::ServerContext *context,
            const syslog::SyslogFilter *request,
            grpc::ServerWriter<::syslog::SyslogMessage> *writer) override;

  grpc::Status GetDataDefinitions(grpc::ServerContext *context,
                                  const google::protobuf::Empty *request,
      grpc::ServerWriter<syslog::SyslogDataDefinition> *writer) override;

  grpc::Status AddNewMessage(grpc::ServerContext* context,
                             const syslog::SyslogMessage* request,
                             google::protobuf::Empty* response) override;
private:
  SyslogRpcServer& server_;

  [[nodiscard]] SqlFilter MakeFilter(const syslog::SyslogFilter& syslog_filter)
      const;

};

} // namespace ods
