/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/


#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <source_location>
#include <boost/program_options.hpp>

#include <util/stringutil.h>
#include <util/logconfig.h>
#include <util/logstream.h>
#include <util/utilfactory.h>
#include <ods/odsfactory.h>
#include "../../src/syslogrpcserver.h"

using namespace util::log;
using namespace util::string;
using namespace std::chrono_literals;
using namespace boost::program_options;
using namespace std::filesystem;


namespace {
std::atomic<bool> kStopMain = false;
std::string kModelFile; // Full path to model file if it exists.
std::string kDbFile;    // Database file name (SQLite).
std::string kDbType = "SQLite";
std::string kConnectionString;
uint16_t kServerPort = 50600;

void StopMainHandler(int signal) {
  kStopMain = true;
  LOG_TRACE() << "Stopping Event Log RPC Server. Signal: " << signal;
  for (size_t count = 0; count < 100 && kStopMain; ++count) {
    std::this_thread::sleep_for(100ms);
  }
}

void FindModelFile() {
  try {
    // First try any standard install location.
    path model(ProgramDataPath());
    model.append("eventlog");
    model.append("model");
    model.append("eventlogdb.xml");
    if (exists(model)) {
      kModelFile = model.string();
      LOG_TRACE() << "Found external model file. File: " << kModelFile;
      return;
    }

    // Try to find it in the repository if running in debug.
    auto location = std::source_location::current();
    path source_file(location.file_name());
    // If the source file exist, then the model file should exists.
    if (!exists(source_file)) {
      return;
    }

    // The repository exists so fetch the model file.
    path internal = source_file.parent_path().parent_path().parent_path();
    internal.append("odsconfigurator");
    internal.append("model");
    internal.append("eventlogdb.xml");
    if (exists(internal)) {
      kModelFile = internal.string();
      LOG_TRACE() << "Found internal model file. File: " << kModelFile;
    }
  } catch (const std::exception &err) {
    LOG_TRACE() << "Model finding error: " << err.what();
  }
}

void SetupLogSystem() {
  // Set log file name to the service name
  auto &log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToFile);
  log_config.SubDir("eventlog/log"); // Hint of relative path
  log_config.BaseName("eventlogrpc"); // Log file name without extension (stem)
  log_config.CreateDefaultLogger(); // Create logger with name 'Default"

  // Log to console if in debug mode. Hint: LOG_TRACE is going to console only.
#ifdef _DEBUG
  std::vector<std::string> empty_list;
  auto log_console =
      util::UtilFactory::CreateLogger(LogType::LogToConsole, empty_list);
  log_config.AddLogger("Console", std::move(log_console));
#endif
  LOG_TRACE() << "Log File created. Path: " << log_config.GetLogFile();
}

bool CheckInputArguments(int nof_arg, char* arg_list[]) {
  // Scan through input arguments
  try {
    std::vector<std::string> config_list;
    options_description desc("Available Options");

    desc.add_options() ("dbtype,D", value<std::string>(&kDbType),
                       "Database type (default SQLite" );
    desc.add_options() ("connection,C", value<std::string>(&kConnectionString),
                       "File name or connection string" );
    desc.add_options() ("port,P", value<uint16_t>(&kServerPort),
                       "Server Port" );
    desc.add_options()("help,H", "Shows available input arguments");

    command_line_parser parser(nof_arg, arg_list);
    parser.options(desc);

    const auto opt = parser.run();
    variables_map vm;
    store(opt, vm);
    notify(vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      kStopMain = false;
      return false;
    }
  } catch (const std::exception &err) {
    LOG_ERROR() << "Fail to parse the input arguments. Error: " << err.what();
    return false;
  }
  return true;
}

void FindSqliteDb() {
  try {
    path db_file(ProgramDataPath());
    db_file.append("eventlog");
    db_file.append("eventlogdb.sqlite");
    if (exists(db_file)) {
      kConnectionString =  db_file.string();
      kDbFile = kConnectionString;
    }
  } catch (const std::exception &err) {
    LOG_TRACE() << "Data Directory error: " << err.what();
  }
}

void CreateSqliteDb() {
  try {
    path db_file(ProgramDataPath());
    db_file.append("eventlog");
    create_directories(db_file);
    db_file.append("eventlogdb.sqlite");
    auto database = ods::OdsFactory::CreateDatabase(ods::DbType::TypeSqlite);
    database->ConnectionInfo(db_file.string());
    ods::IModel model;
    const auto read = model.ReadModel(kModelFile);
    if (!read) {
      throw std::runtime_error("Failed to read the model file.");
    }
    LOG_TRACE() << "Read the model file. File: " << kModelFile;
    const auto create = database->Create(model);
    if (!create) {
      throw std::runtime_error("Failed to create the database.");
    }
    kDbFile = database->ConnectionInfo();
    kConnectionString = database->ConnectionInfo();
    LOG_TRACE() << "Created the database. File: " << kDbFile;
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to create the SQLite database. Error: "
      << err.what();
  }
}

}  // namespace


int main(int nof_arg, char *arg_list[]) {
  signal(SIGTERM, StopMainHandler);
  signal(SIGABRT, StopMainHandler);
#if (_MSC_VER)
  signal(SIGABRT_COMPAT, StopMainHandler);
  signal(SIGBREAK, StopMainHandler);
#endif
  SetupLogSystem();
  if (!CheckInputArguments(nof_arg, arg_list)) {
    return EXIT_SUCCESS;
  }

  // If no connection string, we cannot connect to the database.
  // Plan A: If the DB type is SQLite, we can try the default data path.
  if (kConnectionString.empty() && IEquals(kDbType, "SQLite")) {
    FindSqliteDb();
  }

  // Plan B: If SQLite, create database. Tricky to find the model file.
  // It shall be in <Data Path>/model/eventlogdb.xml at delivery or
  // in the repository if started internally.
  if (kConnectionString.empty() && IEquals(kDbType, "SQLite")) {
    FindModelFile();
    LOG_TRACE() << "Model File: " << kModelFile;
    if (!kModelFile.empty()) {
      CreateSqliteDb();
    }
  }

  if (kConnectionString.empty() ) {
    LOG_ERROR() << "No connection string. Cannot connect to the database.";
    return EXIT_FAILURE;
  }
  LOG_TRACE() << "Database Type: " << kDbType;
  LOG_TRACE() << "Connection String: " << kConnectionString;
  LOG_TRACE() << "RPC Port: " << kServerPort;
  {
    ods::SyslogRpcServer server;
    std::ostringstream arguments;
    arguments << "--connection=\"" << kConnectionString << "\"";
    arguments << " --port=" << kServerPort;
    if (!kDbType.empty()) {
      arguments << " --dbtype=" << kDbType;
    }
    server.Arguments(arguments.str());
    server.Init();
    while (!kStopMain) {
      server.Tick();
      std::this_thread::sleep_for(10ms);
    }
    server.Exit();
  }
  kStopMain = false;

  LOG_TRACE() << "Stopped event RPC daemon";

  auto &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();

  return EXIT_SUCCESS;
}


