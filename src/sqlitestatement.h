/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <vector>
#include <string>
#include <sstream>
#include "ods/icolumn.h"
#include "sqlitedatabase.h"

namespace ods::detail {

class SqliteStatement final {
 public:
  SqliteStatement(sqlite3* database, const std::string& sql);
  ~SqliteStatement();
  SqliteStatement() = delete;

  bool Step();
  void Reset();

  [[nodiscard]] bool IsNull(int column) const;

  void SetValue(int index, const char* value) const;
  void SetValue(int index, bool value) const;
  void SetValue(int index, int64_t value) const;
  void SetValue(int index, double value) const;
  void SetValue(int index, const std::string& value) const;
  void SetValue(int index, const std::vector<uint8_t>& value) const;

  template<typename T>
  void GetValue(int column, T& value) const;

  template<typename T>
  T Value(int column) const;

  template<typename T>
  T Value(const IColumn* column) const;

  [[nodiscard]] int GetColumnIndex(const std::string& column_name) const;
 private:
  sqlite3*  database_ = nullptr;
  sqlite3_stmt* statement_ = nullptr;
};

template<typename T>
void SqliteStatement::GetValue(int column, T& value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }

  if (column < 0) {
    value = {};
    return;
  }

  const auto type = sqlite3_column_type(statement_, column);
  switch (type) {
    case SQLITE_INTEGER: {
      const auto temp = sqlite3_column_int64(statement_, column);
      value = static_cast<T>(temp);
      break;
    }

    case SQLITE_FLOAT: {
      const auto temp = sqlite3_column_double(statement_, column);
      value = static_cast<T>(temp);
      break;
    }

    case SQLITE_TEXT: {
      const auto* temp = sqlite3_column_text(statement_, column);
      const auto bytes = sqlite3_column_bytes(statement_,column);
      std::stringstream val;
      for (int byte = 0; temp != nullptr && byte < bytes; ++byte) {
        val << static_cast<char>(temp[byte]);
      }
      val >> value;
      break;
    }

    case SQLITE_BLOB:
    case SQLITE_NULL:
    default:
      value = {};
      break;
  }
}

template<typename T>
T SqliteStatement::Value(int column) const {
  T value = {};
  GetValue(column, value);
  return value;
}

template<typename T>
T SqliteStatement::Value(const IColumn* column) const {
  T value = {};
  if (column != nullptr) {
    GetValue(GetColumnIndex(column->DatabaseName()), value);
  }
  return value;
}

template<>
void SqliteStatement::GetValue<bool>(int column, bool& value) const;

template<>
void SqliteStatement::GetValue<std::string>(int column, std::string& value) const;

template<>
void SqliteStatement::GetValue<std::vector<uint8_t>>(int column, std::vector<uint8_t>& value) const;

} // end namespace ods::detail


