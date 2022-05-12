/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include "ods/iattribute.h"

using namespace ods;

namespace ods::test {
TEST(OdsItem, TestNormal) { // NOLINT

  IAttribute item1("pelle", true);
  EXPECT_EQ(item1.Value<bool>(), true);
  EXPECT_EQ(item1.Value<int16_t>(), 1);
  EXPECT_EQ(item1.Value<std::string>(), "1");

  IAttribute item2("pelle", 1);
  EXPECT_EQ(item2.Value<bool>(), true);
  EXPECT_EQ(item2.Value<int16_t>(), 1);
  EXPECT_EQ(item2.Value<std::string>(), "1");

  IAttribute item3("pelle", "olle");
  EXPECT_EQ(item3.Value<std::string>(), "olle");

  IAttribute item4("pelle", 1.23F);
  std::cout << item4.Value<std::string>() << std::endl;
  EXPECT_EQ(item4.Value<float>(), 1.23F);

  IAttribute item5("pelle", 1/3.0);
  std::cout << item5.Value<std::string>() << std::endl;
  EXPECT_EQ(item5.Value<double>(), 1/3.0);
}

} // end namespace