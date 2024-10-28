/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <array>
#include "odshelper.h"

namespace ods::test {

TEST(OdsHelper, ConvertToDumpString) {
  constexpr std::string_view test_string = "Olle\n\r~ESC~^^";
  const std::string convert = OdsHelper::ConvertToDumpString(test_string.data());
  std::cout << "Dump string: " << convert << std::endl;
  std::ostringstream dump_line;
  dump_line << convert << "^";
  const auto value_list = OdsHelper::SplitDumpLine(dump_line.str());
  ASSERT_EQ(value_list.size(), 1);
  ASSERT_STREQ(value_list[0].c_str(), test_string.data() );
}

TEST(OdsHelper, SplitDumpline) {
  constexpr std::array<std::string_view, 3> orig_list = {
      "1.23", "Desc 'Daddy '", "ÅÄÖ"
  };
  std::ostringstream temp;
  for (const auto& value : orig_list ) {
    temp << value << "^";
  }
  temp << std::endl;
  const auto dest_list = OdsHelper::SplitDumpLine(temp.str());
  EXPECT_EQ( dest_list.size(), 3);
  for (size_t index = 0; index < orig_list.size(); ++index) {
    EXPECT_STREQ(orig_list[index].data(), dest_list[index].c_str())  << index;
  }
}

TEST(OdsHelper, Base64) {
  for (size_t buffer_size = 0; buffer_size < 1000; ++buffer_size) {
    std::vector<uint8_t> orig_list(buffer_size, 0);
    uint8_t value = 0;
    for (uint8_t& orig : orig_list ) {
      orig = value++;
    }

    const std::string base64 = OdsHelper::ToBase64(orig_list);
    // std::cout << buffer_size << ": " << base64 << std::endl;
    const auto dest_list = OdsHelper::FromBase64(base64);
    ASSERT_EQ(dest_list.size(), buffer_size);
    for (size_t index = 0; index < orig_list.size(); ++index ) {
      EXPECT_EQ(orig_list[index], dest_list[index]) << index;
    }
  }

}

TEST(OdsHelper, HexString) {
  for (size_t buffer_size = 0; buffer_size < 1000; ++buffer_size) {
    std::vector<uint8_t> orig_list(buffer_size, 0);
    uint8_t value = 0;
    for (uint8_t& orig : orig_list ) {
      orig = value++;
    }

    const std::string hex = OdsHelper::ToHexString(orig_list);
    std::cout << buffer_size << ": " << hex << std::endl;
    const auto dest_list = OdsHelper::FromHexString(hex);
    ASSERT_EQ(dest_list.size(), buffer_size);
    for (size_t index = 0; index < orig_list.size(); ++index ) {
      EXPECT_EQ(orig_list[index], dest_list[index]) << index;
    }
  }

}
} // End namespace