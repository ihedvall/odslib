/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <numeric>
#include "util/logconfig.h"
#include "util/logstream.h"
#include "util/timestamp.h"
#include "sqlitedatabase.h"
#include "sqlitestatement.h"
#include "ods/databaseguard.h"
#include "testsqlite.h"

namespace {

constexpr std::string_view kLogRootDir = "o:/test";
constexpr std::string_view kLogFile = "ods_sqlite.log";
constexpr std::string_view kTestDir = "o:/test/ods";
constexpr std::string_view kNotExist = "not_exist.sqlite";
constexpr std::string_view kCreateDb = "CREATE TABLE test_a ("
                                       "id INTEGER PRIMARY KEY,"
                                       "int_value INTEGER, "
                                       "float_value FLOAT,"
                                       "text_value TEXT,"
                                       "blob_value BLOB)";
constexpr std::string_view kSelectDb = "SELECT * FROM test_a";
constexpr std::string_view kInsertDb = "INSERT INTO test_a (int_value, float_value, text_value, blob_value) "
                                       "VALUES(?1, ?2, ?3, ?4)";
constexpr std::string_view kNewDb = "new_db.sqlite";
constexpr std::string_view kWriteDb = "write_db.sqlite";

constexpr std::array<int64_t,4> kIntList = {
    12,
    -12,
    std::numeric_limits<int64_t>::max(),
    std::numeric_limits<int64_t>::min()
};

constexpr std::array<double,4> kFloatList = {
    12.34,
    -12.34,
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::min()
};

constexpr std::array<std::string_view,4> kTextList = {
    "Text34",
    "Text54",
    "",
    ""
};

const std::array<std::vector<uint8_t>, 4>  kBlobList = {
    std::vector<uint8_t> {0,1,2,3},
    std::vector<uint8_t> {0,1,2,3},
    std::vector<uint8_t> {},
    std::vector<uint8_t> {},
};

bool kSkipTest = false;
}

using namespace std::this_thread;
using namespace std::chrono_literals;
using namespace util::log;
using namespace util::time;
using namespace std::filesystem;
using namespace ods::detail;
namespace ods::test {

void TestSqlite::SetUpTestSuite() {
  auto &log_config = LogConfig::Instance();
  log_config.RootDir(kLogRootDir.data());
  log_config.BaseName(kLogFile.data());
  log_config.Type(util::log::LogType::LogToFile);
  log_config.CreateDefaultLogger();
  try {
    std::error_code err;
    remove_all(kTestDir, err);
    create_directories(kTestDir);
  } catch (const std::exception& error) {
    LOG_ERROR() << "Failed to create directories. Error: " << error.what();
    kSkipTest = true;
  }
}

void TestSqlite::TearDownTestSuite() {
  LogConfig &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();
}

TEST_F(TestSqlite, OpenNotExist) {
  if (kSkipTest) {
    GTEST_SKIP();
  }
  std::filesystem::path file(kTestDir);
  file.append(kNotExist);

  SqliteDatabase database(file.string());
  const auto open = database.Open();
  EXPECT_FALSE(open);
  const auto is_open = database.IsOpen();
  EXPECT_FALSE(is_open);
  const auto close = database.Close(false);
  EXPECT_TRUE( close);

}

TEST_F(TestSqlite, CreateDb) {
  if (kSkipTest) {
    GTEST_SKIP();
  }
  std::filesystem::path file(kTestDir);
  file.append(kNewDb);
  try {
    SqliteDatabase database(file.string());

    const auto open = database.OpenEx();
    EXPECT_TRUE(open);
    const auto is_open = database.IsOpen();
    EXPECT_TRUE(is_open);

    database.ExecuteSql(kCreateDb.data());

    const auto close = database.Close(true);
    EXPECT_TRUE(close);

    EXPECT_TRUE(exists(file.string()));

  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}

TEST_F(TestSqlite, WriteAndRead) {
  if (kSkipTest) {
    GTEST_SKIP();
  }


  std::filesystem::path file(kTestDir);
  file.append(kWriteDb);
    // Create DB
  try {
    SqliteDatabase database(file.string());
    EXPECT_TRUE(database.OpenEx());
    database.ExecuteSql(kCreateDb.data());
    database.Close(true);
  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
    // Insert 4 rows
  try {
    SqliteDatabase database(file.string());
    DatabaseGuard guard(database);
    SqliteStatement insert(database.Sqlite3(), kInsertDb.data());

    for (size_t index = 0; index < 4; ++index) {
      insert.SetValue(1, kIntList[index]);
      insert.SetValue(2, kFloatList[index]);
      insert.SetValue(3, kTextList[index].data());
      insert.SetValue(4, kBlobList[index]);

      insert.Step();
      insert.Reset();
    }
  } catch (const std::exception& error) {
    FAIL() << error.what();
  }

  // Read all rows
  try {
    SqliteDatabase database(file.string());
    DatabaseGuard guard(database);
    SqliteStatement read(database.Sqlite3(), kSelectDb.data());

    size_t row = 0;
    int64_t id = 0;
    int64_t int_value = 0;
    std::string text_value;
    std::vector<uint8_t> blob_value;
    double float_value = 0;
    for (auto more = read.Step(); more; more = read.Step()) {
       read.GetValue(0, id);
       EXPECT_GT(id, 0);

      read.GetValue(1, int_value);
      EXPECT_EQ(int_value, kIntList[row]);

      read.GetValue(2, float_value);
      EXPECT_EQ(float_value, kFloatList[row]);

      read.GetValue(3, text_value);
      EXPECT_EQ(text_value, kTextList[row]);
      if (row >= 2) {
        EXPECT_TRUE(read.IsNull(3));
      } else {
        EXPECT_FALSE(read.IsNull(3));
      }

      read.GetValue(4, blob_value);
      EXPECT_EQ(blob_value, kBlobList[row]);
      if (row >= 2) {
        EXPECT_TRUE(read.IsNull(3));
      } else {
        EXPECT_FALSE(read.IsNull(3));
      }

      ++row;
    }
  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}

}
