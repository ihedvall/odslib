/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "testsysloginserter.h"
#include <string>
#include <filesystem>
#include <chrono>
#include <source_location>

#include <util/logconfig.h>
#include <util/logstream.h>
#include <util/utilfactory.h>

#include "ods/imodel.h"
#include "ods/odsfactory.h"

#include "sysloginserter.h"


using namespace std::filesystem;
using namespace std::chrono_literals;
using namespace util;
using namespace util::log;
using namespace workflow;
using namespace util::syslog;
using namespace ods;

namespace {
  std::string kTestDir;
  std::string kModelFile;
  std::string kDbFile;
  constexpr std::string_view kModelPath = "odsconfigurator/model/eventlogdb.xml";
  bool kSkipTest = false;
  std::unique_ptr<IDatabase> kDatabase;
  std::unique_ptr<IListen> kListenServ;
}

namespace ods::test {
void TestSyslogInserter::SetUpTestSuite() {

  auto &log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToConsole);
  log_config.CreateDefaultLogger();
  if (auto* logger = log_config.GetLogger("Default"); logger != nullptr) {
    logger->ShowLocation(false);
  }

  try {
    path root_dir = temp_directory_path();
    LOG_TRACE() << "Temp Path: " << root_dir.string() << std::endl;
    root_dir.append("test");
    root_dir.append("ods");
    // Remove previous test directory is done here not in tear down
    // function, so we can inspect the generated files after the test is done.
    std::error_code err;
    remove_all(root_dir, err); // Don't want any exception here
    create_directories(root_dir);
    kTestDir = root_dir.string();
    LOG_TRACE() << "Test Path: " << kTestDir << std::endl;

    auto location = std::source_location::current();
    const path source_file(location.file_name());
    LOG_TRACE() << "Source File: " << source_file.string() << std::endl;

    // Note 2 parent_path call to step up one level in source directory.
    // This should be the odslib repository path.
    const path source_path = source_file.parent_path().parent_path();
    LOG_TRACE() << "Repository ODSLIB Path: " << source_path.string() << std::endl;

    // Check that model file exists.

    path model_file = source_path;
    model_file.append(kModelPath);
    kModelFile = model_file.string();
    LOG_TRACE() << "Model File: " << kModelFile << std::endl;
    if (!exists(kModelFile)) {
      throw std::runtime_error("Model file doesn't exist.");
    }

  } catch (const std::exception& err) {
    kSkipTest = true;
    LOG_ERROR() << "Skip test is active due to file/dir error. Error: "
              << err.what();
  }



  // Create the database
  try {
    if (kSkipTest) {
      throw std::runtime_error("Didn't create the database due to skip test.");
    }
    path db_file(kTestDir);
    db_file.append("eventlogdb.sqlite");
    kDbFile = db_file.string();
    LOG_TRACE() << "DB Name: " << kDbFile << std::endl;

    kDatabase = OdsFactory::CreateDatabase(DbType::TypeSqlite);
    kDatabase->ConnectionInfo(kDbFile);

    IModel model;
    const auto read = model.ReadModel(kModelFile);
    if (!read) {
      throw std::runtime_error("Failed to read the model file.");
    }
    LOG_TRACE() << "Model Name: " << model.Name() << std::endl;

    const auto create = kDatabase->Create(model);
    if (!create) {
      throw std::runtime_error("Failed to create the database.");
    }
    if (!exists(kDbFile)) {
      throw std::runtime_error("The database file doesn't exist.");
    }
    LOG_INFO() << "Created DB: " << kDbFile << std::endl;
  } catch (const std::exception& err) {
    kSkipTest = true;
    LOG_ERROR() << "Skip test is active due to database creation error. Error: "
      << err.what();
  }

  // Create the LISSQLITE listen logger
  kListenServ = UtilFactory::CreateListen(TypeOfListen::ListenConsoleType, "LISSQLITE");
  const bool start = kListenServ->Start();
  if (!start) {
    LOG_ERROR() << "Failed to start listening for showing SQL operations.";
  }

}

void TestSyslogInserter::TearDownTestSuite() {
  kDatabase.reset();
  kListenServ.reset();

  auto &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();
}

TEST_F(TestSyslogInserter, TestSyslogInserter) {
  if (kSkipTest || !kDatabase) {
    GTEST_SKIP_("Skipped the inserter test");
  }

  for (size_t connect = 0; connect < 2; ++connect) {
    constexpr size_t max_message = 2;
    SyslogInserter inserter(*kDatabase);
    inserter.Init();
    ASSERT_TRUE(inserter.IsOk()) << inserter.LastError();

    for (size_t msg_index = 0; msg_index < max_message; ++msg_index) {
      SyslogMessage msg;
      msg.Severity(SyslogSeverity::Informational);
      std::ostringstream msg_text;
      msg_text << "Test of SyslogInserter " << msg_index + 1;
      msg.Message(msg_text.str());
      EXPECT_TRUE(inserter.AddOneMessage(msg)) << msg_index;
    }

    const size_t count = inserter.GetNofMessages();
    LOG_TRACE() << "Nof Messages: " << count << std::endl;
    EXPECT_GE(count,max_message);
    inserter.Exit();
  }
}

}