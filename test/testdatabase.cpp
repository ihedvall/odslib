/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "testdatabase.h"

#include <filesystem>

#include <util/logconfig.h>
#include <util/logstream.h>


#include "ods/idatabase.h"
#include "ods/odsfactory.h"

using namespace util::log;
using namespace std::filesystem;

namespace {

constexpr std::string_view kDbDir = "k:/test/odslib";

}

namespace ods::test {
DbList TestDatabase::db_list_;
std::string TestDatabase::test_dir_;

void TestDatabase::SetUpTestSuite() {

  // Set up the log to console
  LogConfig& log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToConsole);
  log_config.CreateDefaultLogger();

  // Make a list of available model files.
  try {
    db_list_.clear();
    for (const directory_entry& dir_entry : directory_iterator(kDbDir)) {
      if (!dir_entry.is_regular_file()) {
        continue;
      }
      const path& filename = dir_entry.path();
      if (filename.extension() != ".sqlite") {
        continue;
      }
      const path name = filename.stem();
      db_list_.emplace(name.string(), filename.string());
    }
  } catch (const std::exception& err) {
    LOG_INFO() << "Fail finding model directory. Error: " << err.what();
  }

  // Create a test directory
  try {
    path temp_dir = temp_directory_path();
    temp_dir.append("test");
    temp_dir.append("ods");
    temp_dir.append("db");
    remove_all(temp_dir);
    create_directories(temp_dir);
    test_dir_ = temp_dir.string();
  } catch (const std::exception& err) {
    LOG_ERROR() << "Fail finding temp test directory. Error: " << err.what();
    test_dir_.clear();
  }

  LOG_INFO() << "Running Set Up function.";
}

void TestDatabase::TearDownTestSuite() {
  LOG_INFO() << "Running Tear Down function.";

  // Don't remove any test directory. Remove at next start instead.
  // This is so you can inspect a result after run.

  LogConfig &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();

}

TEST_F(TestDatabase, TestReservedWord) {
  EXPECT_TRUE(IsSqlReservedWord("SELECT"));
  EXPECT_TRUE(IsSqlReservedWord("Select"));
  EXPECT_FALSE(IsSqlReservedWord("SelectA"));
}

TEST_F(TestDatabase, TestMakeBlobString) {
  const std::vector<uint8_t> test_list = { 1, 2, 3, 4};
  const std::string list_text = MakeBlobString(test_list);

  EXPECT_STREQ(list_text.c_str(), "'\\x01020304'");
}

TEST_F(TestDatabase, TestProperties) {
  if (db_list_.empty()) {
    GTEST_SKIP_("No database to test with.");
  }

  const auto first = db_list_.cbegin();
  const std::string& name = first->first;
  const std::string& filename = first->second;

  auto database = OdsFactory::CreateDatabase(DbType::TypeSqlite);
  ASSERT_TRUE(database);

  database->ConnectionInfo(filename);
  EXPECT_EQ(path(database->ConnectionInfo()), path(filename));
  EXPECT_EQ(database->DatabaseType(), DbType::TypeSqlite);

  database->Name("TestDB");
  EXPECT_STREQ(database->Name().c_str(), "TestDB") << name;

  IModel model;
  const bool read_model = database->ReadModel(model);
  EXPECT_TRUE(read_model);
}

TEST_F(TestDatabase, TestReadModel) {
  if (db_list_.empty()) {
    GTEST_SKIP_("No database to test with.");
  }
  for (const auto &[name, filename] : db_list_) {
    std::cout << "Name :" << name << std::endl;
    auto database = OdsFactory::CreateDatabase(DbType::TypeSqlite);
    ASSERT_TRUE(database);
    database->ConnectionInfo(filename);

    IModel model;
    const bool read_model = database->ReadModel(model);
    ASSERT_TRUE(read_model);
    EXPECT_FALSE(model.IsEmpty());

    EXPECT_FALSE(database->IsOpen());
    EXPECT_TRUE(database->Open());
    EXPECT_TRUE(database->IsOpen());
    EXPECT_TRUE(database->Close(true));
    EXPECT_FALSE(database->IsOpen());
  }
}

TEST_F(TestDatabase, TestDumpDatabase) {
  if (db_list_.empty() || test_dir_.empty() ) {
    GTEST_SKIP_("No database to test with.");
  }
  for (const auto& [name, filename] : db_list_) {
    std::cout << "Name :" << name << std::endl;
    auto database = OdsFactory::CreateDatabase(DbType::TypeSqlite);
    ASSERT_TRUE(database);
    database->ConnectionInfo(filename);

    const std::string dump_dir = database->DumpDatabase(test_dir_);
    EXPECT_FALSE(dump_dir.empty());

    auto dump_database = OdsFactory::CreateDatabase(DbType::TypeSqlite);
    ASSERT_TRUE(dump_database);

    // Define where to put the mirror database.
    try {
      const path orig_file(filename);
      const path short_name = orig_file.filename();
      path dest_name(test_dir_);
      dest_name.append(short_name.string());
      dump_database->ConnectionInfo(dest_name.string());
    } catch (const std::exception& err) {
      ADD_FAILURE() << err.what();
    }

    const bool read_dump = dump_database->ReadInDump(dump_dir);
    EXPECT_TRUE(read_dump) << dump_dir;
  }
}

} // End namespace ods::test