/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

#include "ods/itable.h"
#include "ods/iitem.h"

namespace ods {

class OdsHelper {
 public:
  static std::string ConvertToDumpString(const std::string& value);
  static std::vector<std::string> SplitDumpLine(const std::string& input_line);

  static std::vector<uint8_t> FromBase64(const std::string& value);
  static std::string ToBase64(const std::vector<uint8_t>& byte_array);

  static std::vector<uint8_t> FromHexString(const std::string& hex);
  static std::string ToHexString(const std::vector<uint8_t>& byte_array);

  static bool FetchDbtRow(const ITable &table, IItem &row, std::ifstream &file);
};

} // ods

