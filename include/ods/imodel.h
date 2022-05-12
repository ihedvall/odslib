/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <map>
#include <memory>
#include "util/timestamp.h"
#include "util/stringutil.h"
#include "util/ixmlnode.h"

#include "itable.h"
#include "ienum.h"

namespace ods {

class IModel {
 public:
  using TableList = std::map<int64_t, ITable>;
  using EnumList = std::map<std::string, IEnum, util::string::IgnoreCase>;

  IModel() = default;
  virtual ~IModel() = default;

  bool operator == (const IModel& model) const = default;

  [[nodiscard]] const std::string& Name() const {
    return name_;
  }
  void Name(const std::string& name) {
    name_ = name;
  }

  [[nodiscard]] const std::string& Version() const {
    return version_;
  }
  void Version(const std::string& version) {
    version_ = version;
  }

  [[nodiscard]] const std::string& Description() const {
    return description_;
  }
  void Description(const std::string& desc) {
    description_ = desc;
  }

  [[nodiscard]] const std::string& CreatedBy() const {
    return created_by_;
  }
  void CreatedBy(const std::string& creator) {
    created_by_ = creator;
  }

  [[nodiscard]] const std::string& ModifiedBy() const {
    return modified_by_;
  }
  void ModifiedBy(const std::string& creator) {
    modified_by_ = creator;
  }

  [[nodiscard]] const std::string& BaseVersion() const {
    return base_version_;
  }
  void BaseVersion(const std::string& version) {
    base_version_ = version;
  }

  [[nodiscard]] uint64_t Created() const {
    return created_;
  }
  void Created(uint64_t ns1970) {
    created_ = ns1970;
  }

  [[nodiscard]] uint64_t Modified() const {
    return modified_;
  }
  void Modified(uint64_t ns1970) {
    modified_ = ns1970;
  }

  [[nodiscard]] const std::string& SourceName() const {
    return source_name_;
  }
  void SourceName(const std::string& name) {
    source_name_ = name;
  }

  [[nodiscard]] const std::string& SourceType() const {
    return source_type_;
  }
  void SourceType(const std::string& type) {
    source_type_ = type;
  }

  [[nodiscard]] const std::string& SourceInfo() const {
    return source_info_;
  }
  void SourceInfo(const std::string& info) {
    source_info_ = info;
  }

  void AddTable(const ITable& table);
  bool DeleteTable(int64_t application_id);

  void AddEnum(const IEnum& obj);
  void DeleteEnum(const std::string& name);

  [[nodiscard]] int64_t FindNextEnumId() const;
  [[nodiscard]] const EnumList& Enums() const {
    return enum_list_;
  }
  [[nodiscard]] EnumList& Enums() {
    return enum_list_;
  }
  [[nodiscard ]] int64_t FindNextTableId(int64_t parent_id) const;

  [[nodiscard]] const TableList& Tables() const {
    return table_list_;
  }
  void ClearTableList() {
    table_list_.clear();
  }

  [[nodiscard]] std::vector<const ITable*> AllTables() const;

  [[nodiscard]] const IEnum* GetEnum(const std::string& name) const;
  [[nodiscard]] IEnum* GetEnum(const std::string& name);

  [[nodiscard]] const ITable* GetTable(int64_t application_id) const;
  [[nodiscard]] const ITable* GetTableByName(const std::string& name) const;
  [[nodiscard]] const ITable* GetTableByDbName(const std::string& name) const;
  [[nodiscard]] const ITable* GetBaseId(BaseId base) const;

  [[nodiscard]] bool IsEmpty() const;
  [[nodiscard]] bool ReadModel(const std::string& filename);
  [[nodiscard]] bool SaveModel(const std::string& filename) const;
 private:
  std::string name_;
  std::string version_;
  std::string description_;

  std::string created_by_;
  std::string modified_by_;
  std::string base_version_ = "asam35";

  uint64_t created_ =  util::time::TimeStampToNs();
  uint64_t modified_ = util::time::TimeStampToNs();

  std::string source_name_;
  std::string source_type_;
  std::string source_info_;

  TableList table_list_;

  EnumList enum_list_;

  void ReadEnum(const util::xml::IXmlNode& node);
  void ReadTable(const util::xml::IXmlNode& node);
};

} // end namespace



