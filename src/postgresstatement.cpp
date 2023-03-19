/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "postgresstatement.h"
#include <util/logstream.h>
#include <util/timestamp.h>

using namespace util::log;
using namespace util::time;

namespace {
  int HexValue(const char input) {
    switch (input) {
    case '0' : return 0;
    case '1' : return 1;
    case '2' : return 2;
    case '3' : return 3;
    case '4' : return 4;
    case '5' : return 5;
    case '6' : return 6;
    case '7' : return 7;
    case '8' : return 8;
    case '9' : return 9;
    case 'A' : return 10;
    case 'B' : return 11;
    case 'C' : return 12;
    case 'D' : return 13;
    case 'E' : return 14;
    case 'F' : return 15;
    case 'a' : return 10;
    case 'b' : return 11;
    case 'c' : return 12;
    case 'd' : return 13;
    case 'e' : return 14;
    case 'f' : return 15;
    default:
      break;
    }
    return 0;
  }
}

namespace ods::detail {

PostgresStatement::PostgresStatement(PGconn *connection,
                                     const std::string &sql)
: connection_(connection) {
  const auto send = PQsendQuery(connection_, sql.c_str());
  if (send != 1) {
    const auto* msg = PQerrorMessage(connection_);
    const std::string err = msg != nullptr ? msg : "";
    LOG_ERROR() << "Query error: Error: " << err << ", SQL: " << sql;
  } else {
    const auto single = PQsetSingleRowMode(connection_);
    if (single == 0) {
      const auto *msg = PQerrorMessage(connection_);
      const std::string err = msg != nullptr ? msg : "";
      LOG_ERROR() << "Fail single row mode error: Error: " << err
                  << ", SQL: " << sql;
    }
  }
}
PostgresStatement::~PostgresStatement() {
  while (result_ != nullptr) {
    PQclear(result_);
    result_ = PQgetResult(connection_);
  }
}

bool PostgresStatement::Step() {
  if (connection_ == nullptr) {
    return false;
  }
  if (result_ != nullptr) {
    PQclear(result_);
  }
  result_ = PQgetResult(connection_);
  if (result_ != nullptr) {
    const auto status = PQresultStatus(result_);
    if (status == PGRES_SINGLE_TUPLE) {
      return true;
    }
  }
  // PGRES_TUPLE_OK is received at the end
  return false;
}

int PostgresStatement::GetColumnIndex(const std::string &column_name) const {
  if (result_ == nullptr || column_name.empty()) {
    return -1;
  }
  return PQfnumber(result_, column_name.c_str());

}

template <>
void PostgresStatement::GetValue(int column, std::vector<uint8_t>& value) const {
  value.clear();
  if (result_ == nullptr) {
    throw std::runtime_error("No statement result is found. Invalid use.");
  }
  if (PQgetisnull(result_,0, column)) {
    return;
  }
  const auto* text = PQgetvalue(result_, 0, column);
  if (text == nullptr) {
    return;
  }
  std::ostringstream pre_text;
  for (size_t pre = 0; pre < 2 && *text != '\0'; ++pre) {
    pre_text << *text;
    ++text;
  }

  if (pre_text.str() == "\\x") {
    size_t nof_bytes = strlen(text) / 2;
    if (nof_bytes == 0) {
      return;
    }
    value.resize(nof_bytes);
    for (size_t index = 0; index < nof_bytes; ++index) {
      uint8_t val = HexValue(*text) << 4;
      ++text;
      val += HexValue(*text);
      value[index] = val;
      ++text;
    }
  }
}

template <>
void PostgresStatement::GetValue(int column, uint64_t& value) const {
  value = 0;
  if (result_ == nullptr) {
    throw std::runtime_error("No statement result is found. Invalid use.");
  }
  const auto* text = PQgetvalue(result_, 0, column);
  if (text == nullptr) {
    return;
  }
  if (strchr(text, '-') != nullptr) {
    value = IsoTimeToNs(text, false);
  } else {
    try {
      value = std::stoull(text);
    } catch(const std::exception& ) {
    }
  }
}

template <>
void PostgresStatement::GetValue(int column, std::string& value) const {
  value.clear();
  if (result_ == nullptr) {
    throw std::runtime_error("No statement result is found. Invalid use.");
  }
  const auto* text = PQgetvalue(result_, 0, column);
  if (text == nullptr) {
    return;
  }
  value = text;
}
} // namespace ods