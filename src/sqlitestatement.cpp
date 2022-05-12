/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <sstream>
#include <iomanip>
#include <sqlite3.h>
#include <util/stringutil.h>
#include "sqlitestatement.h"

using namespace util::string;
namespace ods::detail {

SqliteStatement::SqliteStatement(sqlite3 *database, const std::string &sql)
: database_(database) {
  if (database_ != nullptr) {
    const char* tail = nullptr;
    const auto stmt = sqlite3_prepare_v2(database_, sql.c_str(),
                                         static_cast<int>(sql.size()), &statement_, &tail);
    if (stmt != SQLITE_OK) {
      std::ostringstream error;
      error << "Prepare statement failed. Error:  " << sqlite3_errmsg(database)
        << ", SQL: " << sql;
      throw std::runtime_error(error.str());
    }

  }
}

SqliteStatement::~SqliteStatement() {
  sqlite3_finalize(statement_);
}

bool SqliteStatement::Step() {
  if (statement_ == nullptr) {
    throw std::runtime_error("No statement created");
  }
  const auto step = sqlite3_step(statement_);
  switch (step) {
    case SQLITE_BUSY:
      throw std::runtime_error("Step statement failed. The database is busy");

    case SQLITE_DONE:
      return false; // Means no more rows

    case SQLITE_ROW:
      return true;

    default: {
      std::ostringstream error;
      error << "Step statement failed. Error: " << sqlite3_errstr(step);
      throw std::runtime_error(error.str());
    }
  }
}

void SqliteStatement::Reset() {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  const auto reset = sqlite3_reset(statement_);
  if (reset != SQLITE_OK) {
    std::ostringstream error;
    error << "Reset statement failed. Error: " << sqlite3_errstr(reset);
    throw std::runtime_error(error.str());
  }
}

void SqliteStatement::SetValue(int index, bool value) const {
  const int64_t temp = value ? 1 : 0;
  SetValue(index, temp);
}

void SqliteStatement::SetValue(int index, int64_t value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  const auto bind = sqlite3_bind_int64(statement_, index,value);
  if (bind != SQLITE_OK) {
    std::ostringstream error;
    error << "Bind (int) statement failed. Error: " << sqlite3_errstr(bind);
    throw std::runtime_error(error.str());
  }
}

void SqliteStatement::SetValue(int index, double value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  const auto bind = sqlite3_bind_double(statement_, index,value);
  if (bind != SQLITE_OK) {
    std::ostringstream error;
    error << "Bind (double) statement failed. Error: " << sqlite3_errstr(bind);
    throw std::runtime_error(error.str());
  }
}

void SqliteStatement::SetValue(int index, const std::string &value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  const auto bind = value.empty() ?
    sqlite3_bind_null(statement_, index) :
    sqlite3_bind_text(statement_, index, value.c_str(),-1, SQLITE_TRANSIENT);
  if (bind != SQLITE_OK) {
    std::ostringstream error;
    error << "Bind (TEXT) statement failed. Error: " << sqlite3_errstr(bind);
    throw std::runtime_error(error.str());
  }
}
void SqliteStatement::SetValue(int index, const char *value) const {
  const std::string temp(value == nullptr ? "" : value);
  SetValue(index, temp);
}

void SqliteStatement::SetValue(int index, const std::vector<uint8_t> &value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  const auto bind = value.empty() ?
                    sqlite3_bind_null(statement_, index) :
                    sqlite3_bind_blob64(statement_, index, value.data(),
                                      value.size(), SQLITE_TRANSIENT);
  if (bind != SQLITE_OK) {
    std::ostringstream error;
    error << "Bind (BLOB) statement failed. Error: " << sqlite3_errstr(bind);
    throw std::runtime_error(error.str());
  }
}

bool SqliteStatement::IsNull(int column) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  const auto type = sqlite3_column_type(statement_, column);
  return type == SQLITE_NULL;
}

int SqliteStatement::GetColumnIndex(const std::string &column_name) const {
  if (statement_ == nullptr || column_name.empty()) {
    return -1;
  }
  const auto count = sqlite3_column_count(statement_);
  for (int column = 0; column < count; ++column) {
    const auto* name = sqlite3_column_name(statement_,column);
    if (name != nullptr && IEquals(column_name, name)) {
      return column;
    }
  }
  return -1;
}



template<>
void SqliteStatement::GetValue(int column, bool& value) const {
  int64_t temp = 0;
  GetValue(column, temp);
  value = temp != 0;
}

template<>
void SqliteStatement::GetValue(int column, std::string& value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  if (column < 0) {
    value.clear();
    return;
  }

  const auto type = sqlite3_column_type(statement_, column);

  switch (type) {
    case SQLITE_INTEGER: {
      const auto temp = sqlite3_column_int64(statement_, column);
      value = std::to_string(temp);
      break;
    }

    case SQLITE_FLOAT: {
      const auto temp = sqlite3_column_double(statement_, column);
      value = std::to_string(temp);
      break;
    }

    case SQLITE_TEXT: {
      const auto* temp = sqlite3_column_text(statement_, column);
      const auto bytes = sqlite3_column_bytes(statement_,column);
      std::ostringstream val;
      for (int byte = 0; temp != nullptr && byte < bytes; ++byte) {
        const char in_char =  static_cast<char>(temp[byte]);
        if (in_char == '\0') {
          break;
        }
        val << in_char;
      }
      value = val.str();
      break;
    }

    case SQLITE_BLOB: {
      const auto* temp = sqlite3_column_blob(statement_, column);
      const auto bytes = sqlite3_column_bytes(statement_,column);
      std::ostringstream val;

      for (int byte = 0; temp != nullptr && byte < bytes; ++byte) {
        const auto* list = static_cast<const uint8_t*>(temp);
        val << std::setw(2) << std::setfill('0')
            << std::hex << static_cast<int>(list[byte]);
        if (byte > 30) {
          val << "...";
          break;
        }
      }
      value = val.str();
      break;
    }

    case SQLITE_NULL:
    default:
      value.clear();
      break;
  }
}


template<>
void SqliteStatement::GetValue(int column, std::vector<uint8_t>& value) const {
  if (statement_ == nullptr) {
    throw std::runtime_error("Statement is null");
  }
  if (column < 0) {
    value.clear();
    return;
  }
  const auto type = sqlite3_column_type(statement_, column);

  switch (type) {

    case SQLITE_TEXT: {
      const auto* temp = sqlite3_column_text(statement_, column);
      const auto bytes = sqlite3_column_bytes(statement_,column);
      if (bytes > 0 ) {
        value.resize(bytes, 0);
        memcpy(value.data(), temp, bytes);
      }
      break;
    }

    case SQLITE_BLOB: {
      const auto* temp = sqlite3_column_blob(statement_, column);
      const auto bytes = sqlite3_column_bytes(statement_,column);
      if (bytes > 0 ) {
        value.resize(bytes, 0);
        memcpy(value.data(), temp, bytes);
      }
      break;
    }

    case SQLITE_INTEGER:
    case SQLITE_FLOAT:
    case SQLITE_NULL:
    default:
      value.clear();
      break;
  }

}

}
