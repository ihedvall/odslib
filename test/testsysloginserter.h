/*
* Copyright 2026 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <gtest/gtest.h>

namespace ods::test {

class TestSyslogInserter : public testing::Test {
  public:
    static void SetUpTestSuite();
    static void TearDownTestSuite();
  protected:

};

} // namespace ods
