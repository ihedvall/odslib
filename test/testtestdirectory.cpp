/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <filesystem>
#include <gtest/gtest.h>
#include "util/logconfig.h"
#include "testdirectory.h"


using namespace ods::detail;
using namespace util::log;

namespace {

constexpr std::string_view kTestDir = "o:/test/ods";
constexpr std::string_view kTestDb = "o:/test/ods/test_db.sqlite";
constexpr std::string_view kModelFile = "D:\\projects\\TP V5\\comtest4\\ctbas\\common\\odsbase\\edlct5model.xml";

bool SkipTest() {
  auto &log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToConsole);
  log_config.CreateDefaultLogger();

  try {
    std::error_code err;
    std::filesystem::remove_all(kTestDir, err);
    std::filesystem::create_directories(kTestDir);
    if (!std::filesystem::exists(kModelFile) ) {
      std::cout << "Failed to create directories." << std::endl;
      return true;
    }
  } catch (const std::exception& error) {
    std::cout << "Failed to create directories. Error: " << error.what() << std::endl;
    return true;
  }
  return false;
}


}

namespace ods::test {

TEST( TestDirectory, Init) {
  if (SkipTest()) {
    GTEST_SKIP();
  }

  TestDirectory test_dir;
  test_dir.Name("TestDb");
  test_dir.RootDir(kTestDir.data());
  test_dir.DbFileName(kTestDb.data());
  test_dir.ModelFileName(kModelFile.data());
  EXPECT_TRUE(test_dir.Init());
}


} // end namespace