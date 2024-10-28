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

using ModelList = std::map<std::string, std::string, util::string::IgnoreCase>;

class TestModel : public testing::Test  {
 public:

  static void SetUpTestSuite();
  static void TearDownTestSuite();
 protected:


  static ModelList model_list_;

  static std::string test_dir_;
};

} // ods


