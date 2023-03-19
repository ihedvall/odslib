/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "testpostgres.h"

#include <string_view>
#include <array>
#include <filesystem>

#include <util/logstream.h>
#include <util/logconfig.h>
#include <util/stringutil.h>
#include <util/timestamp.h>

#include "postgresdb.h"
#include "postgresstatement.h"
#include "ods/databaseguard.h"

using namespace util::log;
using namespace util::string;
using namespace util::time;

using namespace std::filesystem;
using namespace ods::detail;

namespace {

constexpr std::string_view kLogRootDir = "o:/test";
constexpr std::string_view kLogFile = "ods_postgres.log";
constexpr std::string_view kTestDir = "o:/test/ods";
constexpr std::string_view kConnectInfo =
    "dbname=EventLogDb user=postgres password=postgres";

constexpr std::string_view kDropTable = "DROP TABLE IF EXISTS test_a";
constexpr std::string_view kSelectDb = "SELECT * FROM test_a";

bool kSkipTest = false;

}

namespace ods::test {
void TestPostgres::SetUpTestSuite() {
  auto &log_config = LogConfig::Instance();
  log_config.RootDir(kLogRootDir.data());
  log_config.BaseName(kLogFile.data());
  log_config.Type(util::log::LogType::LogToFile);
  log_config.CreateDefaultLogger();
  try {
    std::error_code err;
    remove_all(kTestDir, err);
    create_directories(kTestDir);

    PostgresDb database;
    database.ConnectionInfo(kConnectInfo.data());
    DatabaseGuard guard(database);
    kSkipTest = !guard.IsOk();

  } catch (const std::exception& error) {
    LOG_ERROR() << "Failed to create directories. Error: " << error.what();
    kSkipTest = true;
  }
}

void TestPostgres::TearDownTestSuite() {
  LogConfig &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();
}

TEST_F(TestPostgres, CreateDb) {
  constexpr std::string_view kCreateDb = "CREATE TABLE IF NOT EXISTS test_a ("
                                         "id bigserial PRIMARY KEY,"
                                         "int_value bigint, "
                                         "float_value real,"
                                         "text_value text,"
                                         "blob_value bytea)";
  if (kSkipTest) {
    GTEST_SKIP();
  }

  try {
    PostgresDb database;
    database.ConnectionInfo(kConnectInfo.data());

    const auto open = database.Open();
    EXPECT_TRUE(open);
    const auto is_open = database.IsOpen();
    EXPECT_TRUE(is_open);
    database.ExecuteSql(kDropTable.data());
    database.ExecuteSql(kCreateDb.data());

    const auto close = database.Close(true);
    EXPECT_TRUE(close);

  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}

TEST_F(TestPostgres, TestNumber) {
  constexpr std::string_view kCreateDb = "CREATE TABLE IF NOT EXISTS test_a ("
                                         "idx bigserial PRIMARY KEY,"
                                         "short_value smallint, "
                                         "int_value integer,"
                                         "int64_value bigint,"
                                         "float_value real,"
                                         "double_value double precision)";
  constexpr std::array<int16_t, 4> kShortList = {
      12,
      -12,
      std::numeric_limits<int16_t>::max(),
      std::numeric_limits<int16_t>::min()
  };

  constexpr std::array<int32_t, 4> kIntList = {
      12,
      -12,
      std::numeric_limits<int32_t>::max(),
      std::numeric_limits<int32_t>::min()
  };

  constexpr std::array<int64_t, 4> kInt64List = {
      12,
      -12,
      std::numeric_limits<int64_t>::max(),
      std::numeric_limits<int64_t>::min()
  };

  constexpr std::array<float, 4> kFloatList = {
      12.34,
      -12.34,
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::min()
  };

  constexpr std::array<double, 4> kDoubleList = {
      12.34,
      -12.34,
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::min()
  };

  if (kSkipTest) {
    GTEST_SKIP();
  }
  try {
    PostgresDb database;
    database.ConnectionInfo(kConnectInfo.data());
    DatabaseGuard guard(database);
    EXPECT_TRUE(guard.IsOk());

    database.ExecuteSql(kDropTable.data());
    database.ExecuteSql(kCreateDb.data());
    for (size_t index = 0; index < 4; ++index) {
      std::ostringstream sql;
      sql << "INSERT INTO test_a "
          << "(short_value,int_value,int64_value,float_value,double_value) "
          << "VALUES ("
          << kShortList[index] << ","
          << kIntList[index] << ","
          << kInt64List[index] << ","
          << FloatToString(kFloatList[index]) << ","
          << DoubleToString(kDoubleList[index]) << ") "
          << "RETURNING idx";
      const auto idx = database.ExecuteSql(sql.str());
      EXPECT_EQ(idx,index + 1);
      std::cout << idx << ": " << sql.str() << std::endl;
    }
    std::cout << kSelectDb << std::endl;
    PostgresStatement select(database.Connection(), kSelectDb.data());
    int row = 0;
    for (bool more = select.Step(); more ; more = select.Step()) {
      const auto idx = select.Value<int64_t>("idx");
      const auto short_value = select.Value<int16_t>("short_value");
      const auto int_value = select.Value<int32_t>("int_value");
      const auto int64_value = select.Value<int64_t>("int64_value");
      const auto float_value = select.Value<float>("float_value");
      const auto double_value = select.Value<double>("double_value");
      EXPECT_EQ(idx, row + 1);
      EXPECT_EQ(short_value, kShortList[row]);
      EXPECT_EQ(int_value, kIntList[row]);
      EXPECT_EQ(int64_value, kInt64List[row]);
      EXPECT_FLOAT_EQ(float_value, kFloatList[row]);
      EXPECT_DOUBLE_EQ(double_value, kDoubleList[row]);
      ++row;
    }

  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}

TEST_F(TestPostgres, TestText) {
  constexpr std::string_view kCreateDb = "CREATE TABLE IF NOT EXISTS test_a ("
                                         "idx serial PRIMARY KEY,"
                                         "text1_value varchar, "
                                         "text2_value text,"
                                         "text3_value char(2))";
  if (kSkipTest) {
    GTEST_SKIP();
  }

  try {
    PostgresDb database;
    database.ConnectionInfo(kConnectInfo.data());
    DatabaseGuard guard(database);
    EXPECT_TRUE(guard.IsOk());

    database.ExecuteSql(kDropTable.data());
    database.ExecuteSql(kCreateDb.data());

    std::ostringstream sql;
    sql << "INSERT INTO test_a "
        << "(text1_value,text2_value,text3_value) "
        << "VALUES ("
        << "'olle'" << ","
        << "'pelle'" << ","
        << "'PT'" << ") "
        << "RETURNING idx";
    {
      auto idx = database.ExecuteSql(sql.str());
      EXPECT_EQ(idx, 1);
      std::cout << idx << ": " << sql.str() << std::endl;
    }
    std::cout << kSelectDb << std::endl;
    PostgresStatement select(database.Connection(), kSelectDb.data());
    int row = 0;
    for (bool more = select.Step(); more ; more = select.Step()) {
      const auto idx = select.Value<int64_t>("idx");
      const auto text1_value = select.Value<std::string>("TEXT1_VALUE");
      const auto text2_value = select.Value<std::string>("text2_value");
      const auto text3_value = select.Value<std::string>("text3_value");

      EXPECT_EQ(idx, row + 1);
      EXPECT_STREQ(text1_value.c_str(), "olle");
      EXPECT_STREQ(text2_value.c_str(), "pelle");
      EXPECT_STREQ(text3_value.c_str(), "PT");
      ++row;
    }
    EXPECT_EQ(row,1);
  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}

TEST_F(TestPostgres, TestBlob) {
  constexpr std::string_view kCreateDb = "CREATE TABLE IF NOT EXISTS test_a ("
                                         "idx serial PRIMARY KEY,"
                                         "blob_value bytea)";
  const std::array<std::vector<uint8_t>, 4> kBlobList = {
      std::vector<uint8_t>{0, 1, 2, 3, 55, 255},
      std::vector<uint8_t>{0, 1, 2, 3},
      std::vector<uint8_t>{0},
      std::vector<uint8_t>{},
  };

  if (kSkipTest) {
    GTEST_SKIP();
  }

  try {
    PostgresDb database;
    database.ConnectionInfo(kConnectInfo.data());
    DatabaseGuard guard(database);
    EXPECT_TRUE(guard.IsOk());

    database.ExecuteSql(kDropTable.data());
    database.ExecuteSql(kCreateDb.data());

    for (const auto& blob : kBlobList) {
      std::ostringstream sql;
      sql << "INSERT INTO test_a "
          << "(blob_value) "
          << "VALUES (" << MakeBlobString(blob) << ") "
          << "RETURNING idx";
      auto idx = database.ExecuteSql(sql.str());
      std::cout << idx << ": " << sql.str() << std::endl;
    }

    std::cout << kSelectDb << std::endl;
    PostgresStatement select(database.Connection(), kSelectDb.data());
    int row = 0;
    for (bool more = select.Step(); more ; more = select.Step()) {
      const auto idx = select.Value<int64_t>("idx");
      const auto blob_value = select.Value<std::vector<uint8_t>>("blob_value");
      EXPECT_EQ(idx, row + 1);
      EXPECT_EQ(blob_value, kBlobList[row]);
      ++row;
    }
    EXPECT_EQ(row,kBlobList.size());
  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}

TEST_F(TestPostgres, TestTimestamp) {
  constexpr std::string_view kCreateDb = "CREATE TABLE IF NOT EXISTS test_a ("
                                         "idx serial PRIMARY KEY,"
                                         "time_value timestamp(6) with time zone)";
  const std::array<uint64_t,2> kTimeList = {
      0,
      TimeStampToNs()
  };

  if (kSkipTest) {
    GTEST_SKIP();
  }

  try {
    PostgresDb database;
    database.ConnectionInfo(kConnectInfo.data());
    DatabaseGuard guard(database);
    EXPECT_TRUE(guard.IsOk());

    database.ExecuteSql(kDropTable.data());
    database.ExecuteSql(kCreateDb.data());

    for (const auto& ns1970 : kTimeList) {
      std::ostringstream sql;
      sql << "INSERT INTO test_a "
          << "(time_value) "
          << "VALUES ('" << NsToIsoTime(ns1970, 2) << "') "
          << "RETURNING idx";
      auto idx = database.ExecuteSql(sql.str());
      std::cout << idx << ": " << sql.str() << std::endl;
    }

    std::cout << kSelectDb << std::endl;
    PostgresStatement select(database.Connection(), kSelectDb.data());
    int row = 0;
    for (bool more = select.Step(); more ; more = select.Step()) {
      const auto idx = select.Value<int64_t>("idx");

      const auto time_string = select.Value<std::string>("time_value");
      std::cout << idx << " : " << time_string << std::endl;

      const auto time_value = select.Value<uint64_t>("time_value");
      EXPECT_EQ(idx, row + 1);
      EXPECT_EQ(time_value / 1000, kTimeList[row] / 1000);
      ++row;
    }
    EXPECT_EQ(row,kTimeList.size());
  } catch (const std::exception& error) {
    FAIL() << error.what();
  }
}
} // namespace ods::test