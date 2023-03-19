/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <gtest/gtest.h>
#include <libpq-fe.h>

namespace ods::test {

TEST(LibPq, TestBasic) {
  std::cout << "DEFAULTS" << std::endl;
  {
    auto *connect_info = PQconndefaults();
    for (size_t index = 0;
         connect_info != nullptr && connect_info[index].keyword != nullptr;
         ++index) {
      const auto &info = connect_info[index];
      if (info.val == nullptr) {
        continue ;
      }
      std::cout << info.keyword << ": "
                << (info.val != nullptr ? info.val : "") << std::endl;
    }

    PQconninfoFree(connect_info);
  }
  std::cout << std::endl << "CONNECTION STRING INFO" << std::endl;

  std::string_view connection_string =
      "dbname=EventLogDb user=postgres password=postgres";

  {
    char *error_msg = nullptr;
    auto *connect_info = PQconninfoParse(connection_string.data(), &error_msg);
    if (error_msg != nullptr) {
      std::cout << "Error: " << error_msg << std::endl;
      free(error_msg);
    }
    for (size_t index = 0;
         connect_info != nullptr && connect_info[index].keyword != nullptr;
         ++index) {
      const auto &info = connect_info[index];
      if (info.val == nullptr) {
        continue;
      }
      std::cout << info.keyword << ": "
                << (info.val != nullptr ? info.val : "") << std::endl;
    }

    PQconninfoFree(connect_info);
  }
  std::cout << std::endl << "CONNECTION URL INFO" << std::endl;
  std::string_view connection_url =
      "postgresql://postgres:postgres@/EventLogDb";

  {
    char *error_msg = nullptr;
    auto *connect_info = PQconninfoParse(connection_url.data(), &error_msg);
    if (error_msg != nullptr) {
      std::cout << "Error: " << error_msg << std::endl;
      free(error_msg);
    }
    for (size_t index = 0;
         connect_info != nullptr && connect_info[index].keyword != nullptr;
         ++index) {
      const auto &info = connect_info[index];
      if (info.val == nullptr) {
        continue;
      }
      std::cout << info.keyword << ": "
                << (info.val != nullptr ? info.val : "") << std::endl;
    }

    PQconninfoFree(connect_info);
  }
  auto* connect = PQconnectdb(connection_string.data());
  ASSERT_TRUE(connect != nullptr);

  const auto status = PQstatus(connect);
  EXPECT_EQ(status, CONNECTION_OK) << status << std::endl;

  std::cout << std::endl << "CONNECTION INFO" << std::endl;
  {
    auto *connect_info = PQconninfo(connect);
    for (size_t index = 0;
         connect_info != nullptr && connect_info[index].keyword != nullptr;
         ++index) {
      const auto &info = connect_info[index];
      if (info.val == nullptr) {
        continue;
      }
      std::cout << info.keyword << ": "
                << (info.val != nullptr ? info.val : "") << std::endl;
    }

    PQconninfoFree(connect_info);
  }

  PQfinish(connect);

}

} // namespace ods::test
