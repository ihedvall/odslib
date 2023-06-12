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

/** \brief Type of where condition.
 *
 */
enum class SqlCondition {
  Equal,        ///< WHERE column = value ('value' for strings)
  EqualIgnoreCase, ///< WHERE LOWER(column) = LOWER('value')
  Greater,      ///< WHERE column \> value
  Less,         ///< WHERE column \< value
  GreaterEQ,    ///< WHERE column \>= value
  LessEQ,       ///< WHERE column \<= value
  NotEqual,     ///< WHERE column \<\> value
  NotEqualIgnoreCase,///< WHERE LOWER(column \<\> LOWER('value')
  Like,         ///< WHERE column LIKE value
  NotLike,      ///< WHERE column NOT LIKE value
  In,           ///< WHERE column IN (val1, val2...)
  InIgnoreCase, ///< WHERE LOWER(column) IN (LOWER('val1'), LOWER('val2')...)
  NotIn,        ///< WHERE column NOT IN (val1, val2...)
  OrderByNone,  ///< ORDER BY column
  OrderByAsc,   ///< ORDER BY column ASC
  OrderByDesc,  ///< ORDER BY column DESC
  LimitNofRows, ///< LIMIT value
  LimitOffset,  ///< OFFSET value
};

struct SqlFilterItem {
  std::string  column_name;
  SqlCondition condition;
  std::string  value;
};

[[nodiscard]] std::string WildcardToSql(const std::string& wildcard);

class SqlFilter {
 public:

   /** \brief Simple function that handle compare conditions.
    *
    * Simple function that adds a where statement that compares
    * value. You can use this method for IdNameMap, ItemList. The later
    * type assume some sort of IN statement.
    * @tparam T Type of value
    * @param column Reference to the column.
    * @param condition Type of compare.
    * @param value The value.
    */
  template <typename T>
  void AddWhere(const IColumn& column, SqlCondition condition, const T &value);
  void AddWhereSelect(const IColumn& column, SqlCondition condition,
                    const ITable& parent, const SqlFilter& parent_filter);
  void AddOrder(const IColumn& column,
                SqlCondition condition = SqlCondition::OrderByNone,
                const std::string&expression = {});

  void AddLimit(SqlCondition condition , uint64_t value);

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
void SqlFilter::AddWhere<bool>(const IColumn &column, SqlCondition condition,
                               const bool &value);

template<>
void SqlFilter::AddWhere<const char*>(const IColumn &column,
                                       SqlCondition condition,
                                       const char* const &value);

template<>
void SqlFilter::AddWhere<std::vector<int64_t>>(const IColumn &column,
                                               SqlCondition condition,
    const std::vector<int64_t>& value);

template<>
void SqlFilter::AddWhere<IdNameMap>(const IColumn &column,
                                    SqlCondition condition,
                                    const IdNameMap& value);

template<>
void SqlFilter::AddWhere<ItemList>(const IColumn &column,
                                   SqlCondition condition,
                                    const ItemList& value);
static SqlFilter kSqlEmptyFilter = SqlFilter();

} // end namespace



