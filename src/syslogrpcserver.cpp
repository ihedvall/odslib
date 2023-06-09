//
// Created by ihedv on 2023-06-04.
//

#include "syslogrpcserver.h"
#include <sstream>
#include <boost/program_options.hpp>
#include <grpcpp/grpcpp.h>
#include <util/logstream.h>
#include "ods/odsfactory.h"
#include "ods/databaseguard.h"
#include "syslogservice.h"

#include "template_names.icc"

using namespace boost::program_options;
using namespace util::string;
using namespace util::syslog;
using namespace util::log;
using namespace grpc;

namespace {
constexpr uint16_t kDefaultSyslogServerPort = 50600; ///< Default event port
}
namespace ods {
SyslogRpcServer::SyslogRpcServer() {
  Name(kSyslogRpcServer.data());
  Template(kSyslogRpcServer.data());
  Description("Implements a gRPC server against the syslog database.");
  std::ostringstream temp;
  temp << "--slot=" << data_slot_ << " ";
  temp << "--dbtype=" << db_type_ << " ";
  temp << "--connection=\"" << connection_string_ << "\" ";
  temp << "--port=" << server_port_;
  Arguments(temp.str());
}

SyslogRpcServer::SyslogRpcServer(const workflow::IRunner &source)
    : IRunner(source) {
  Template(kSyslogRpcServer.data());
  ParseArguments();
}

SyslogRpcServer::~SyslogRpcServer() {
  StopThread();
}

void SyslogRpcServer::ParseArguments() {
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
    desc.add_options() ("port,P",
                       value<uint16_t>(&server_port_),
                       "Server Port" );
    const auto arg_list = split_winmain(arguments);
    basic_command_line_parser parser(arg_list);
    parser.options(desc);
    const auto opt = parser.run();
    variables_map var_list;
    store(opt,var_list);
    notify(var_list);
    IsOk(true);
  } catch( const std::exception& err) {
    LastError("Init error.");
    LOG_ERROR() << "Init error, Name: " << Name() << ", Error: " << err.what();
    IsOk(false);
  }
  if (server_port_ == 0) {
    server_port_ = kDefaultSyslogServerPort;
  }
}

void SyslogRpcServer::Init() {
  IRunner::Init();
  const auto* temp = GetRunnerByTemplateName(kSyslogInserter.data());
  inserter_ = temp != nullptr ? dynamic_cast<const SyslogInserter*>(temp) :
                              nullptr;
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
  StartThread();
}

void SyslogRpcServer::Exit() {
  StopThread();
  IRunner::Exit();
  database_.reset();
  inserter_ = nullptr;
}

SyslogMessage SyslogRpcServer::LastMessage() const {
  if (inserter_ != nullptr) {
    // The inserter stores the last message. This is much faster than open the
    // database
    return inserter_->LastMessage();
  }
  SyslogMessage message;
  if (database_) {
    auto* table = model_.GetTableByName("Syslog");
    auto* id_column = table != nullptr ? table->GetColumnByBaseName("id") :
                                       nullptr;
    DatabaseGuard lock(*database_);
    if (lock.IsOk() && table != nullptr && id_column != nullptr) {
      // The SQL: select * from data where user='me' order by id desc limit 1
      SqlFilter filter;
      filter.AddOrder(*id_column, SqlCondition::OrderByDesc);
      filter.AddLimit(SqlCondition::LimitNofRows, 1);
      ItemList message_list;
      try {
        database_->FetchItemList(*table, message_list, filter );
        if (!message_list.empty()) {
          auto& item = message_list.front();
          message.Index(item->BaseValue<int64_t>("id"));
          message.Message(item->BaseValue<std::string>("name"));
          message.Timestamp(item->BaseValue<uint64_t>("date"));
          message.Severity(static_cast<SyslogSeverity>(
                                        item->Value<uint8_t>("Severity")));
          message.Facility(static_cast<SyslogFacility>(
              item->Value<uint8_t>("Facility")));
          message.ProcessId(item->Value<std::string>("ProcessID"));
          message.MessageId(item->Value<std::string>("MessageID"));
        }
      } catch( const std::exception&) {}
    }
  }
  return message;
}

void SyslogRpcServer::StartThread() {
  StopThread();
  service_thread_ = std::thread(&SyslogRpcServer::WorkerThread, this);
}

void SyslogRpcServer::StopThread() {
  if (server_) {
    server_->Shutdown();
  }
  if (service_thread_.joinable()) {
    service_thread_.join();
  }
}

void SyslogRpcServer::WorkerThread() {
  try {
    std::ostringstream address;
    address << "0.0.0.0:" << server_port_;
    SyslogService service(*this);
    ServerBuilder builder;

    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(address.str(), InsecureServerCredentials());

    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);

    // Finally assemble the server.
    LOG_TRACE() << "RPC server building and starting. Name: " << Name();
    server_ = std::move(builder.BuildAndStart());
    LOG_TRACE() << "RPC server started. Name: " << Name();
    IsOk(true);
    server_->Wait();
    LOG_TRACE() << "RPC server stopped. Name: " << Name();
  } catch (const std::exception& err) {
    LastError("Start RPC error");
    IsOk(false);
    LOG_ERROR() << "RPC start error. Name: " << Name()
      << ", Error: " << err.what();
  }
  server_.reset();
}


} // namespace ods