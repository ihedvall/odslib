/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <gtest/gtest.h>
#pragma once
namespace ods::test {

class TestPostgres : public testing::Test {
public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();
};

} // namespace ods::test
