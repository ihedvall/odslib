/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>

#include <locale>
#include <vector>

#include <ods/iattribute.h>

namespace ods::test {
TEST(IAttribute, TestProperties) {
  IAttribute attr;

  attr.Name("ApplicationName");
  EXPECT_STREQ(attr.Name().c_str(), "ApplicationName");

  attr.BaseName("BaseName");
  EXPECT_STREQ(attr.BaseName().c_str(), "BaseName");

  EXPECT_FALSE(attr.IsValueUnsigned());
  EXPECT_TRUE(attr.IsValueEmpty());

  IAttribute attr1("ApplicationName", "Value");
  EXPECT_STREQ(attr1.Name().c_str(), "ApplicationName");
  EXPECT_STREQ(attr1.Value<std::string>().c_str(), "Value");

  IAttribute attr2("ApplicationName", "BaseName", "Value");
  EXPECT_STREQ(attr2.Name().c_str(), "ApplicationName");
  EXPECT_STREQ(attr2.BaseName().c_str(), "BaseName");
  EXPECT_STREQ(attr2.Value<std::string>().c_str(), "Value");
}

TEST(IAttribute, TestUnsigned) {
  IAttribute attr;
  EXPECT_FALSE(attr.IsValueUnsigned());

  attr.Value(1.23);
  EXPECT_FALSE(attr.IsValueUnsigned());

  attr.Value("2002:01:01");
  EXPECT_FALSE(attr.IsValueUnsigned());

  attr.Value(0);
  EXPECT_TRUE(attr.IsValueUnsigned());

  attr.Value(10);
  EXPECT_TRUE(attr.IsValueUnsigned());

  attr.Value(-110);
  EXPECT_FALSE(attr.IsValueUnsigned());
}

TEST(IAttribute, TestIsEmpty) {
  IAttribute attr;
  EXPECT_TRUE(attr.IsValueEmpty());

  attr.Value(1.23);
  EXPECT_FALSE(attr.IsValueEmpty());

  attr.Value("");
  EXPECT_TRUE(attr.IsValueEmpty());
}

TEST(IAttribute, TestNumber) {
  IAttribute attr;

  attr.Value(123);
  EXPECT_EQ(attr.Value<int64_t>(), 123);

  attr.Value(-123);
  EXPECT_EQ(attr.Value<int64_t>(), -123);

  attr.Value("-1234");
  EXPECT_EQ(attr.Value<int64_t>(), -1234);

  attr.Value(123'000);
  EXPECT_EQ(attr.Value<int64_t>(), 123'000);

  attr.Value(-1.234F);
  EXPECT_FLOAT_EQ(attr.Value<float>(), -1.234F);

  attr.Value(-1.23456);
  EXPECT_DOUBLE_EQ(attr.Value<double>(), -1.23456);
}

TEST(IAttribute, TestLocaleNumber) {
  std::setlocale(LC_ALL,"de_DE");

  IAttribute attr;

  attr.Value(-1.234);
  EXPECT_FLOAT_EQ(attr.Value<float>(), -1.234F);

  attr.Value(-1.23456);
  EXPECT_DOUBLE_EQ(attr.Value<double>(), -1.23456);

  std::setlocale(LC_ALL,"");
}

TEST(IAttribute, TestArray) {
  const std::vector<uint8_t> byte_array = {1,2,3,4};
  IAttribute attr;
  attr.Value(byte_array);
  const auto dest_array = attr.Value<std::vector<uint8_t>>();
  ASSERT_EQ(byte_array.size(),dest_array.size());
  for (size_t index = 0; index < byte_array.size(); ++index) {
    EXPECT_EQ(byte_array[index], dest_array[index]) << index;
  }
}
} // End namespace ods::test