/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <gtest/gtest.h>
#include <string>
#include <map>
#include <util/stringutil.h>
namespace ods::test {

using DbList = std::map<std::string, std::string, util::string::IgnoreCase>;

class TestDatabase : public testing::Test  {
 public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();
 protected:
  static DbList db_list_;
  static std::string test_dir_;
};

} // End namespace ods::test

