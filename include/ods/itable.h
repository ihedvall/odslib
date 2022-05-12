/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "odsdef.h"
#include "icolumn.h"

namespace ods {

IColumn CreateDefaultColumn(BaseId base_id, const std::string& base_name);

class ITable {
 public:

  using SubTableList = std::map<int64_t, ITable>;
  using ColumnList = std::vector<IColumn>;

  ITable() = default;
  virtual ~ITable() = default;

  bool operator == (const ITable& table) const = default;

  [[nodiscard]] int64_t ApplicationId() const {
    return application_id_;
  }
  void ApplicationId(int64_t id) {
    application_id_ = id;
  }

  [[nodiscard]] int64_t ParentId() const {
    return parent_id_;
  }

  void ParentId(int64_t id);

  [[nodiscard]] BaseId BaseId() const {
    return base_id_;
  }
  void BaseId(ods::BaseId id) {
    base_id_ = id;
  }

  [[nodiscard]] int64_t SecurityMode() const {
    return security_mode_;
  }
  void SecurityMode(int64_t mode) {
    security_mode_ = mode;
  }

  [[nodiscard]] std::string ApplicationName() const {
    return application_name_;
  }
  void ApplicationName(const std::string& name) {
    application_name_ = name;
  }

  [[nodiscard]] std::string DatabaseName() const {
    return database_name_;
  }
  void DatabaseName(const std::string& name) {
    database_name_ = name;
  }

  [[nodiscard]] std::string Description() const {
    return description_;
  }
  void Description(const std::string& text) {
    description_ = text;
  }

  [[nodiscard]] const SubTableList& SubTables() const {
    return sub_table_list_;
  }
  [[nodiscard]] SubTableList& SubTables() {
    return sub_table_list_;
  }

  [[nodiscard]] const ColumnList & Columns() const {
    return column_list_;
  }

  [[nodiscard]] ColumnList & Columns() {
    return column_list_;
  }

  void AddSubTable(const ITable& table);
  void AddColumn(const IColumn& column);

  bool DeleteSubTable(int64_t application_id);
  void DeleteColumn(const std::string& name);

  [[nodiscard]] int64_t FindNextColumnId() const;
  [[nodiscard]] ColumnList MakeUniqueList() const;

  [[nodiscard]] const ITable* GetTable(int64_t application_id) const;
  [[nodiscard]] const ITable* GetTableByName(const std::string& name) const;
  [[nodiscard]] const ITable* GetTableByDbName(const std::string& name) const;
  [[nodiscard]] const ITable* GetBaseId(ods::BaseId base) const;

  [[nodiscard]] const IColumn* GetColumnByName(const std::string& name) const;
  [[nodiscard]] const IColumn* GetColumnByDbName(const std::string& name) const;
  [[nodiscard]] const IColumn* GetColumnByBaseName(const std::string& name) const;

 private:

  int64_t  application_id_ = 0; ///< Application ID. Shall be > 0.
  int64_t  parent_id_ = 0; ///< Parent table ID. 0 means no parent.
  ods::BaseId base_id_ = BaseId::AoAny; ///< ODS base ID.

  std::string application_name_; ///< Application name (max 30 characters).
  std::string database_name_; ///< Database table name (max 30 characters).
  std::string description_; ///< Descriptive information about the table. Optional information.

  int64_t security_mode_ = 0; ///< Security mode. Not used.

  SubTableList sub_table_list_;
  ColumnList column_list_; ///< List of columns


};

} // end namespace ods
