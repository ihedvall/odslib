/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include "util/stringutil.h"
#include "ods/itable.h"
#include "ods/baseattribute.h"
using namespace util::string;

namespace ods {

IColumn CreateDefaultColumn(BaseId base_id, const std::string &base_name) {
  IColumn column;
  if (!base_name.empty()) {
    const auto base_list = GetBaseAttributeList(base_id);
    const auto itr = std::ranges::find_if(base_list, [&] (const auto& base) {
      return util::string::IEquals(base.BaseName(), base_name);
    });
    if (itr != base_list.cend()) {
      const IColumn& temp = *itr;
      column = temp;
    }
    column.BaseName(base_name);
  }
  return column;
}

void ods::ITable::AddColumn(const IColumn& column) {
  column_list_.push_back(column);
}

void ITable::AddSubTable(const ITable& table) {
  sub_table_list_.insert({table.ApplicationId(), table});
}

const ITable *ITable::GetTable(int64_t application_id) const {
  if (application_id == ApplicationId()) {
    return this;
  }
  const auto itr = sub_table_list_.find(application_id);
  if (itr != sub_table_list_.cend()) {
    return &itr->second;
  }

  for (const auto& sub : sub_table_list_) {
    const auto* find = sub.second.GetTable(application_id);
    if (find != nullptr) {
      return find;
    }
  }
  return nullptr;
}

const ITable *ITable::GetBaseId(ods::BaseId base) const {
  if (BaseId() == base) {
    return this;
  }
  for (const auto& itr : sub_table_list_) {
    const auto* table = itr.second.GetBaseId(base);
    if (table != nullptr) {
      return table;
    }
  }
  return nullptr;
}

const ITable *ITable::GetTableByName(const std::string& name) const {
  if (util::string::IEquals(name, application_name_)) {
    return this;
  }
  for (const auto& itr : sub_table_list_) {
    const auto& table = itr.second;
    const auto* find = table.GetTableByName(name);
    if (find != nullptr) {
      return find;
    }
  }
  return nullptr;
}

const ITable *ITable::GetTableByDbName(const std::string& name) const {
  if (util::string::IEquals(name, database_name_)) {
    return this;
  }
  for (const auto& itr : sub_table_list_) {
    const auto& table = itr.second;
    const auto* find = table.GetTableByDbName(name);
    if (find != nullptr) {
      return find;
    }
  }
  return nullptr;
}

const IColumn *ITable::GetColumnByName(const std::string &name) const {
  const auto itr = std::ranges::find_if(column_list_, [&] (const auto& column) {
    return util::string::IEquals(name,column.ApplicationName());
  });
  return itr == column_list_.cend() ? GetColumnByDbName(name) : &(*itr);
}

const IColumn *ITable::GetColumnByDbName(const std::string &name) const {
   const auto itr = std::ranges::find_if(column_list_, [&] (const auto& column) {
    return util::string::IEquals(name,column.DatabaseName());
  });
  return itr == column_list_.cend() ? GetColumnByBaseName(name) : &(*itr);
}

const IColumn *ITable::GetColumnByBaseName(const std::string &name) const {
  const auto itr = std::ranges::find_if(column_list_, [&] (const auto& column) {
    return util::string::IEquals(name,column.BaseName());
  });
  return itr == column_list_.cend() ? nullptr : &(*itr);
}

bool ITable::DeleteSubTable(int64_t application_id) {
  for (auto itr = sub_table_list_.begin(); itr != sub_table_list_.end(); ++itr) {
    if (itr->second.ApplicationId() == application_id) {
      sub_table_list_.erase(itr);
      return true;
    }
    const auto sub = itr->second.DeleteSubTable(application_id);
    if (sub) {
      return true;
    }
  }
  return false;
}

void ITable::DeleteColumn(const std::string &name) {
  auto itr = std::ranges::find_if(column_list_, [&] (const auto& column) {
    return util::string::IEquals(name,column.ApplicationName());
  });
  if (itr != column_list_.end()) {
    column_list_.erase(itr);
  }
}

void ITable::ParentId(int64_t id) {
  // Scan through its columns and replace parent id references
  std::ranges::for_each(column_list_, [&] (auto& column) {
    const auto old_id = column.ReferenceId();
    if (old_id > 0 && old_id == parent_id_) {
      column.ReferenceId(id);
    }
  });
  parent_id_ = id;
}

int64_t ITable::FindNextColumnId() const {
  for (int64_t next = 1; next < static_cast<int64_t>(column_list_.size() + 10); ++next) {
    const auto exist = std::ranges::any_of(column_list_, [&] (const auto& column) {
    return column.ColumnId() == next;
    });
    if (!exist) {
      return next;
    }
  }
  return 0;
}


ITable::ColumnList ITable::MakeUniqueList() const {
  std::vector<IColumn> unique_list;
  for (const auto& column : column_list_) {
    if (column.DatabaseName().empty() || IEquals(column.BaseName(), "id") || !column.Unique()) {
      continue;
    }
    unique_list.emplace_back(column);
  }
  return unique_list;
}


} // end namespace

