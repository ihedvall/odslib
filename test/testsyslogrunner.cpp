/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "testsyslogrunner.h"
#include <string>
#include <filesystem>
#include <chrono>
#include <util/logconfig.h>
#include "ods/odsfactory.h"
#include "ods/imodel.h"
#include "sysloginserter.h"
#include <source_location>

using namespace std::filesystem;
using namespace std::chrono_literals;
using namespace util::log;
using namespace workflow;
using namespace util::syslog;


namespace {
  std::string kTestLogDir;
  std::string kTestDir;
  std::string kModelFile;
  std::string kDbFile;
  constexpr std::string_view kModelPath =
      "odsconfigurator/model/eventlogdb.xml";
  bool kSkipTest = false;
  std::unique_ptr<ods::IDatabase> kDatabase;
}

namespace ods::test {
void TestSyslogRunner::SetUpTestSuite() {
  try {
    path root_dir = temp_directory_path();
    std::cout << "Temp Path: " << root_dir.string() << std::endl;
    root_dir.append("test");
    create_directories(root_dir);
    kTestLogDir = root_dir.string();
    std::cout << "Log Path: " << kTestLogDir << std::endl;

    root_dir.append("ods");
    // Remove previous test directory is done here not in tear down
    // function, so we can inspect the generated files after the test is done.
    std::error_code err;
    remove_all(root_dir, err); // Don't want any exception here
    create_directories(root_dir);
    kTestDir = root_dir.string();
    std::cout << "Test Path: " << kTestDir << std::endl;

    auto location = std::source_location::current();
    path source_file(location.file_name());
    std::cout << "Source File: " << source_file.string() << std::endl;

    // Note 2 parent_path call to step up one level in source directory
    path source_path = source_file.parent_path().parent_path();
    std::cout << "Source Path: " << source_path.string() << std::endl;

    // Check that model file exists.

    path model_file = source_path;
    model_file.append(kModelPath);
    kModelFile = model_file.string();
    std::cout << "Model File: " << kModelFile << std::endl;
    if (!exists(kModelFile)) {
      throw std::runtime_error("Model file doesn't exist.");
    }
  } catch (const std::exception& err) {
    kSkipTest = true;
    std::cout << "Skip test is active due to file/dir error. Error: "
              << err.what();
  }
  auto &log_config = LogConfig::Instance();
  log_config.RootDir(kTestLogDir);
  log_config.BaseName("test_syslog_runner.log");
  log_config.Type(util::log::LogType::LogToFile);
  log_config.CreateDefaultLogger();

  log_config.AddLogger("Console", LogType::LogToConsole, {});

  // Create the database
  try {
    if (kSkipTest) {
      throw std::runtime_error("Didn't create the database due to skip test.");
    }
    path db_file(kTestDir);
    db_file.append("eventlogdb.sqlite");
    kDbFile = db_file.string();
    std::cout << "DB Name: " << kDbFile << std::endl;

    kDatabase = std::move(OdsFactory::CreateDatabase(DbType::TypeSqlite));
    kDatabase->ConnectionInfo(kDbFile);

    IModel model;
    const auto read = model.ReadModel(kModelFile);
    if (!read) {
      throw std::runtime_error("Failed to read the model file.");
    }
    std::cout << "Model Name: " << model.Name() << std::endl;

    const auto create = kDatabase->Create(model);
    if (!create) {
      throw std::runtime_error("Failed to create the database.");
    }
    if (!exists(kDbFile)) {
      throw std::runtime_error("The database file doesn't exist.");
    }
    std::cout << "Created: " << model.Name() << std::endl;
  } catch (const std::exception& err) {
    kSkipTest = true;
    std::cout << "Skip test is active due to database creation error. Error: "
      << err.what();
  }
}

void TestSyslogRunner::TearDownTestSuite() {
  kDatabase.reset();
  auto &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();
}

TEST_F(TestSyslogRunner, TestInsert) {
  if (kSkipTest || !kDatabase) {
    GTEST_SKIP_("Skipped the inserter test");
  }
  SyslogInserter inserter;
  std::ostringstream arg;
  arg << "--connection=\"" << kDatabase->ConnectionInfo();
  inserter.Arguments(arg.str());
  inserter.Init();
  ASSERT_TRUE(inserter.IsOk()) << inserter.LastError();
  SyslogMessage msg1;
  msg1.Severity(SyslogSeverity::Informational);
  msg1.Message("Msg1");
  EXPECT_TRUE(inserter.AddOneMessage(msg1));
  const auto count = inserter.GetNofMessages();
  std::cout << "Nof Messages: " << count << std::endl;
  EXPECT_GT(inserter.GetNofMessages(),0);
  inserter.Exit();
}

} // namespace ods::test