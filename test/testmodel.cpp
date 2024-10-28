/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "testmodel.h"

#include <filesystem>

#include <util/logconfig.h>
#include <util/logstream.h>
#include <util/timestamp.h>

#include "ods/imodel.h"

using namespace util::log;
using namespace util::time;
using namespace std::filesystem;

namespace {
  constexpr std::string_view kModelDir = "k:/test/odsmodel";
}

namespace ods::test {
ModelList TestModel::model_list_;
std::string TestModel::test_dir_;

void TestModel::SetUpTestSuite() {

  // Set up the log to console
  LogConfig& log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToConsole);
  log_config.CreateDefaultLogger();

  // Make a list of available model files.
  try {
    model_list_.clear();
    for (const directory_entry& dir_entry : recursive_directory_iterator(kModelDir)) {
      if (!dir_entry.is_regular_file()) {
        continue;
      }
      const path& filename = dir_entry.path();
      const path name = filename.stem();
      model_list_.emplace(name.string(), filename.string());
    }
  } catch (const std::exception& err) {
    LOG_INFO() << "Fail finding model directory. Error: " << err.what();
  }

  // Create a test directory
  try {
    path temp_dir = temp_directory_path();
    temp_dir.append("test");
    temp_dir.append("ods");
    temp_dir.append("model");
    remove_all(temp_dir);
    create_directories(temp_dir);
    test_dir_ = temp_dir.string();
  } catch (const std::exception& err) {
    LOG_ERROR() << "Fail finding temp test directory. Error: " << err.what();
    test_dir_.clear();
  }

  LOG_INFO() << "Running Set Up function.";

}

void TestModel::TearDownTestSuite() {
  LOG_INFO() << "Running Tear Down function.";

  // Don't remove any test directory. Remove at next start instead.
  // This is so you can inspect a result after run.

  LogConfig &log_config = LogConfig::Instance();
  log_config.DeleteLogChain();
}

TEST_F(TestModel, ModelProperties) {
  IModel model;

  model.Name("ModelName");
  EXPECT_STREQ(model.Name().c_str(), "ModelName");

  model.Version("ModelVersion");
  EXPECT_STREQ(model.Version().c_str(), "ModelVersion");

  model.Description("ModelDescription");
  EXPECT_STREQ(model.Description().c_str(), "ModelDescription");

  model.CreatedBy("Freddy Kruger");
  EXPECT_STREQ(model.CreatedBy().c_str(), "Freddy Kruger");

  model.ModifiedBy("Jack the Ripper");
  EXPECT_STREQ(model.ModifiedBy().c_str(), "Jack the Ripper");

  model.BaseVersion("BaseVersion");
  EXPECT_STREQ(model.BaseVersion().c_str(), "BaseVersion");

  const uint64_t created = TimeStampToNs();
  model.Created(created);
  EXPECT_EQ(model.Created(), created);

  const uint64_t modified = TimeStampToNs();
  model.Modified(modified);
  EXPECT_EQ(model.Modified(), modified);

  model.SourceName("SourceName");
  EXPECT_STREQ(model.SourceName().c_str(), "SourceName");

  model.SourceType("SourceType");
  EXPECT_STREQ(model.SourceType().c_str(), "SourceType");

  model.SourceInfo("SourceInfo");
  EXPECT_STREQ(model.SourceInfo().c_str(), "SourceInfo");
}

TEST_F(TestModel, ModelRead) {
  if (model_list_.empty()) {
    GTEST_SKIP_("No models to read");
  }

  for (const auto& [name, filename] : model_list_) {
    IModel model;
    const bool read = model.ReadModel(filename);
    EXPECT_TRUE(read) << name;

    EXPECT_GT(model.Tables().size(), 0) << name;
    EXPECT_FALSE(model.IsEmpty() ) << name;

    std::cout << name << std::endl;
  }
}

TEST_F(TestModel, ModelSave) {
  if (model_list_.empty() || test_dir_.empty()) {
    GTEST_SKIP_("No models to read");
  }

  for (const auto& [name, filename] : model_list_) {
    IModel orig;
    const bool read = orig.ReadModel(filename);
    EXPECT_TRUE(read) << name;

    std::string dest_name;
    try {
      std::string short_name = name + ".xml";
      path dest(test_dir_);
      dest.append(short_name);
      dest_name = dest.string();
    } catch (const std::exception& err) {
      ADD_FAILURE() << name << ", Error; " << err.what();
      continue;
    }
    const bool save = orig.SaveModel(dest_name);
    EXPECT_TRUE(save) << name;

    IModel compare;
    const bool comp = compare.ReadModel(dest_name);
    EXPECT_TRUE(comp) << name;
    EXPECT_EQ(compare, orig) << name;

    std::cout << name << std::endl;
  }
}

} // ods