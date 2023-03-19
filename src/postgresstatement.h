/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <libpq-fe.h>
#include "ods/icolumn.h"

namespace ods::detail {

class PostgresStatement final {
public:
  PostgresStatement() = delete;
  PostgresStatement(PGconn* connection, const std::string& sql);
  virtual ~PostgresStatement();

  bool Step();

  template<typename T>
  void GetValue(int column, T& value) const;

  template<typename T>
  T Value(int column) const;

  template<typename T>
  T Value(const IColumn* column) const;

  template<typename T>
  T Value(const std::string& column) const;

  [[nodiscard]] int GetColumnIndex(const std::string& column_name) const;
private:
  PGconn* connection_ = nullptr;
  PGresult* result_ = nullptr;
};

template <typename T>
void PostgresStatement::GetValue(int column, T& value) const {
  value = {};
  if (result_ == nullptr) {
    throw std::runtime_error("No statement result is found. Invalid use.");
  }
  const auto* text = PQgetvalue(result_, 0, column);
  if (text != nullptr) {
    std::istringstream conv(text);
    conv >> value;
  }
}

template <>
void PostgresStatement::GetValue(int column, std::vector<uint8_t>& value) const;

template <>
void PostgresStatement::GetValue(int column, uint64_t& value) const;

template <>
void PostgresStatement::GetValue(int column, std::string& value) const;

template <typename T>
T PostgresStatement::Value(int column) const {
  T temp = {};
  GetValue(column, temp);
  return temp;
}

template<typename T>
T PostgresStatement::Value(const IColumn* column) const {
  T value = {};
  if ( column != nullptr ) {
    const auto index = GetColumnIndex(column->DatabaseName());
    GetValue(index, value);
  }
  return value;
}

template<typename T>
T PostgresStatement::Value(const std::string& column) const {
  T value = {};
  const auto index = GetColumnIndex(column);
  GetValue(index, value);
  return value;
}

} // namespace ods::detail
