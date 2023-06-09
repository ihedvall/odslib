/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <gtest/gtest.h>
#include <memory>
#include <ods/idatabase.h>

namespace ods::test {

class TestSyslogRunner : public testing::Test {
public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();
protected:

};

} // namespace ods
