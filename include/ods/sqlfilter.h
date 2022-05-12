/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "ods/odsdef.h"
#include "ods/icolumn.h"
#include "ods/iitem.h"

namespace ods {

enum class SqlCondition {
  Equal,
  EqualIgnoreCase,
  Greater,
  Less,
  GreaterEQ,
  LessEQ,
  NotEqual,
  NotEqualIgnoreCase,
  Like,
  NotLike,
  StartsWith,
  EndsWith,
  In,
  InIgnoreCase,
  NotIn,
  OrderByNone,
  OrderByAsc,
  OrderByDesc,
  LimitNofRows,
  LimitOffset,
};

struct SqlFilterItem {
  std::string  column_name;
  SqlCondition condition;
  std::string  value;
};

[[nodiscard]] std::string WildcardToSql(const std::string& wildcard);

class SqlFilter {
 public:

  template <typename T>
  void AddWhere(const IColumn& column, SqlCondition condition, const T &value);
  void AddOrder(const IColumn& column, SqlCondition condition = SqlCondition::OrderByNone, const std::string&expression = {});
  void AddLimit(SqlCondition condition , int64_t value);

  [[nodiscard]] std::string GetWhereStatement() const;

  [[nodiscard]] bool IsEmpty() const;;

 protected:
  [[nodiscard]] virtual std::string MakeSqlText(const std::string& text) const;
  [[nodiscard]] virtual std::string MakeDateText(const std::string& time_string) const;
 private:
  std::vector<SqlFilterItem> where_list_;
  std::vector<SqlFilterItem> order_by_list_;
  std::vector<SqlFilterItem> limit_list_;

  [[nodiscard]] std::string GetOrderByStatement() const;
  [[nodiscard]] std::string GetLimitStatement() const;
};

template<typename T>
void SqlFilter::AddWhere(const IColumn &column, SqlCondition condition, const T& value) {
  if (column.DatabaseName().empty()) {
    return;
  }

  std::ostringstream temp;
  temp << value;
  if (temp.str().empty() && !column.Obligatory()) {
    temp << "null";
  }

  if (column.DataType() == DataType::DtString) {
    const std::string val = MakeSqlText(temp.str());
    SqlFilterItem item = { column.DatabaseName(), condition,val};
    where_list_.emplace_back(item);
  } else if (column.DataType() == DataType::DtDate) {
    const std::string val = MakeDateText(temp.str());
    SqlFilterItem item = { column.DatabaseName(), condition,val};
    where_list_.emplace_back(item);
  } else {
    SqlFilterItem item = {column.DatabaseName(), condition, temp.str()};
    where_list_.emplace_back(item);
  }
}

template<>
void SqlFilter::AddWhere<bool>(const IColumn &column, SqlCondition condition, const bool &value);

template<>
void SqlFilter::AddWhere<const char*>(const IColumn &column, SqlCondition condition, const char* const &value);

template<>
void SqlFilter::AddWhere<std::vector<int64_t>>(const IColumn &column, SqlCondition condition,
    const std::vector<int64_t>& value);

template<>
void SqlFilter::AddWhere<IdNameMap>(const IColumn &column, SqlCondition condition,
                                                       const IdNameMap& value);

template<>
void SqlFilter::AddWhere<ItemList>(const IColumn &column, SqlCondition condition,
                                    const ItemList& value);
static SqlFilter kSqlEmptyFilter = SqlFilter();

} // end namespace



