/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include "sqlite3.h"
#include "ods/idatabase.h"

namespace ods {

bool IsSqlReservedWord(const std::string &word) {
  return sqlite3_keyword_check(word.c_str(), word.size()) > 0;
}
}

