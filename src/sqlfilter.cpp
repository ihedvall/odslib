/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <algorithm>
#include <sqlite3.h>
#include <boost/lexical_cast.hpp>
#include <util/stringutil.h>
#include <util/timestamp.h>
#include "ods/sqlfilter.h"


namespace {
const char *ConditionString(ods::SqlCondition condition) {
  switch (condition) {
  case ods::SqlCondition::Greater:
    return " > ";
  case ods::SqlCondition::Less:
    return " < ";
  case ods::SqlCondition::GreaterEQ:
    return " >= ";
  case ods::SqlCondition::LessEQ:
    return " <= ";

  case ods::SqlCondition::NotEqual:
  case ods::SqlCondition::NotEqualIgnoreCase:
    return " <> ";

  case ods::SqlCondition::Like:
    return " LIKE ";

  case ods::SqlCondition::NotLike:
    return " NOT LIKE ";

  case ods::SqlCondition::In:
  case ods::SqlCondition::InIgnoreCase:
    return " IN ";

  case ods::SqlCondition::NotIn:
    return " NOT IN ";

  default:
    break;
  }
  return " = ";
}

} // end namespace

namespace ods {

template <>
void SqlFilter::AddWhere<bool>(const IColumn &column, SqlCondition condition,
                               const bool &value) {
  AddWhere(column, condition, value ? 1 : 0);
}

template <>
void SqlFilter::AddWhere<const char *>(const IColumn &column,
                                       SqlCondition condition,
                                       const char *const &value) {
  const std::string val = value == nullptr ? "" : value;
  AddWhere(column, condition, val);
}

template <>
void SqlFilter::AddWhere<std::vector<int64_t>>(
    const IColumn &column, SqlCondition condition,
    const std::vector<int64_t> &value) {
  if (column.DatabaseName().empty()) {
    return;
  }

  std::ostringstream temp;
  temp << "(";
  size_t count = 0;
  for (auto index : value) {
    if (count > 0) {
      temp << ",";
    }
    temp << index;
    ++count;
  }
  temp << ")";

  SqlFilterItem item = {column.DatabaseName(), condition, temp.str()};
  where_list_.emplace_back(item);
}

template <>
void SqlFilter::AddWhere<IdNameMap>(const IColumn &column,
                                    SqlCondition condition,
                                    const IdNameMap &value) {
  if (column.DatabaseName().empty()) {
    return;
  }
  std::ostringstream temp;
  switch (condition) {
  case SqlCondition::InIgnoreCase: // (LOWER('val1'), LOWER('val2'))
  {
    // Assume that it's the name part of the that shall be used
    temp << "(";
    size_t count = 0;
    for (const auto &[idx, name] : value) {
      if (count > 0) {
        temp << ",";
      }
      temp << "LOWER(" << MakeSqlText(name) << ")";
      ++count;
    }
    temp << ")";
    break;
  }

  case SqlCondition::NotIn: // (val1,val2...)
  case SqlCondition::In: {
    // Assume that it's the name part of the that shall be used
    temp << "(";
    size_t count = 0;
    for (const auto &[idx, name] : value) {
      if (count > 0) {
        temp << ",";
      }
      if (column.DataType() == DataType::DtString) {
        temp << MakeSqlText(name);
      } else if (column.DataType() == DataType::DtDate) {
        temp << MakeDateText(name); // Maybe little wierd data type
      } else {
        temp << idx;
      }
      ++count;
    }
    temp << ")";
    break;
  }

  default: // No support
    return;
  }
  where_list_.emplace_back(column.DatabaseName(), condition, temp.str());
}

template <>
void SqlFilter::AddWhere<ItemList>(const IColumn &column,
                                   SqlCondition condition,
                                   const ItemList &value) {
  if (column.DatabaseName().empty()) {
    return;
  }
  std::ostringstream temp;
  switch (condition) {
  case SqlCondition::InIgnoreCase: // (LOWER('val1'), LOWER('val2'))
  {

    temp << "(";
    size_t count = 0;
    for (const auto &item : value) {
      if (count > 0) {
        temp << ",";
      }
      const auto val = item->Value<std::string>(column.ApplicationName());
      temp << "LOWER(" << MakeSqlText(val) << ")";
      ++count;
    }
    temp << ")";
    break;
  }

  case SqlCondition::NotIn: // (val1,val2...)
  case SqlCondition::In: {
    // Assume that it's the name part of the that shall be used
    temp << "(";
    size_t count = 0;
    for (const auto &item : value) {
      if (count > 0) {
        temp << ",";
      }
      const auto val = item->Value<std::string>(column.ApplicationName());
      if (column.DataType() == DataType::DtString) {
        temp << MakeSqlText(val);
      } else if (column.DataType() == DataType::DtDate) {
        temp << MakeDateText(val); // Maybe little wierd data type
      } else {
        temp << val;
      }
      ++count;
    }
    temp << ")";
    break;
  }

  default: // No support
    return;
  }
  where_list_.emplace_back(column.DatabaseName(), condition, temp.str());
}

void SqlFilter::AddWhereSelect(const IColumn &column, SqlCondition condition,
                               const ITable &parent,
                               const SqlFilter &parent_filter) {
  if (column.DatabaseName().empty()) {
    return;
  }

  // Create the inner select statement. In reality, it's always the index
  // column that is used.

  std::ostringstream temp;
  switch (condition) {
    // (SELECT LOWER('name') FROM table WHERE...)
  case SqlCondition::InIgnoreCase:
    if (const auto *name_column = parent.GetColumnByBaseName("name");
        name_column != nullptr) {
      temp << "(SELECT LOWER(" << name_column->DatabaseName() << ") FROM "
           << parent.DatabaseName() << " " << parent_filter.GetWhereStatement()
           << ")";
    }
    break;

  case SqlCondition::NotIn: // (val1,val2...)
  case SqlCondition::In:
    if (column.DataType() == DataType::DtString) {
      if (const auto *name_column = parent.GetColumnByBaseName("name");
          name_column != nullptr) {
        temp << "(SELECT LOWER(" << name_column->DatabaseName() << ") FROM "
             << parent.DatabaseName() << " "
             << parent_filter.GetWhereStatement() << ")";
      }
    } else {
      if (const auto *id_column = parent.GetColumnByBaseName("id");
          id_column != nullptr) {
        temp << "(SELECT " << id_column->DatabaseName() << " FROM "
             << parent.DatabaseName() << " "
             << parent_filter.GetWhereStatement() << ")";
      }
    }
    break;

  default: // No support
    return;
  }
  if (temp.str().empty()) {
    return;
  }
  where_list_.emplace_back(column.DatabaseName(), condition, temp.str());
}

std::string WildcardToSql(const std::string &wildcard) {
  std::ostringstream temp;
  if (wildcard.empty()) {
    temp << '%';
  }

  for (const char cin : wildcard) {
    switch (cin) {
    case '*':
      temp << '%';
      break;

    case '?':
      temp << '_';
      break;

    case '!':
      temp << '^';
      break;

    default:
      temp << cin;
      break;
    }
  }
  return temp.str();
}

std::string SqlFilter::GetWhereStatement() const {
  if (where_list_.empty()) {
    return GetOrderByStatement();
  }

  size_t count = 0;
  std::ostringstream where;
  where << "WHERE ";
  for (const auto &item : where_list_) {
    const bool ignore_case =
        item.condition == SqlCondition::EqualIgnoreCase ||
        item.condition == SqlCondition::NotEqualIgnoreCase ||
        item.condition == SqlCondition::InIgnoreCase;

    if (count > 0) {
      where << " AND ";
    }
    if (ignore_case) {
      where << "LOWER(" << item.column_name << ")";
    } else {
      where << item.column_name;
    }

    where << ConditionString(item.condition);

    if (ignore_case) {
      where << "LOWER(" << item.value << ")";
    } else {
      where << item.value;
    }

    ++count;
  }

  if (!order_by_list_.empty()) {
    where << " " << GetOrderByStatement(); // The function also adds limit
  }
  return where.str();
}

void SqlFilter::AddOrder(const IColumn &column, SqlCondition condition,
                         const std::string &expression) {
  if (column.DatabaseName().empty()) {
    return;
  }
  SqlFilterItem item{column.DatabaseName(), condition, expression};
  order_by_list_.emplace_back(item);
}

std::string SqlFilter::GetOrderByStatement() const {
  if (order_by_list_.empty()) {
    return GetLimitStatement();
  }
  size_t count = 0;
  std::ostringstream order_by;
  order_by << "ORDER BY ";
  for (const auto &item : order_by_list_) {
    if (count > 0) {
      order_by << ", ";
    }
    order_by << item.column_name;
    if (!item.value.empty()) {
      order_by << " " << item.value;
    }
    switch (item.condition) {
    case SqlCondition::OrderByAsc:
      order_by << " ASC";
      break;

    case SqlCondition::OrderByDesc:
      order_by << " DESC";
      break;

    case SqlCondition::OrderByNone:
    default:
      break;
    }

    ++count;
  }
  if (!limit_list_.empty()) {
    order_by << " " << GetLimitStatement();
  }
  return order_by.str();
}

void SqlFilter::AddLimit(SqlCondition condition, uint64_t value) {
  SqlFilterItem item{"", condition, std::to_string(value)};
  limit_list_.emplace_back(item);
}

std::string SqlFilter::GetLimitStatement() const {
  if (limit_list_.empty()) {
    return "";
  }
  std::ostringstream limit;
  for (const auto &item : limit_list_) {
    switch (item.condition) {
    case SqlCondition::LimitNofRows:
      limit << "LIMIT " << item.value;
      break;

    case SqlCondition::LimitOffset:
      limit << "OFFSET " << item.value;
      break;

    default:
      break;
    }
  }

  return limit.str();
}

std::string SqlFilter::MakeSqlText(const std::string &text) const {
  if (util::string::IEquals(text, "null")) {
    return text;
  }

  auto *temp = sqlite3_mprintf("%Q", text.c_str());
  std::string ret = temp;
  sqlite3_free(temp);
  return ret;
}

std::string SqlFilter::MakeDateText(const std::string &time_string) const {
  // Is either a ISO date string or ns since 1970
  const bool is_nano_sec = std::ranges::all_of(
      time_string, [](const char in_byte) { return isdigit(in_byte); });
  if (is_nano_sec) {
    return MakeSqlText(
        util::time::NsToIsoTime(boost::lexical_cast<uint64_t>(time_string)));
  }
  return MakeSqlText(time_string);
}

bool SqlFilter::IsEmpty() const {
  return where_list_.empty() && order_by_list_.empty() && limit_list_.empty();
}

} // end namespace