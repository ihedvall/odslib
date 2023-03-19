/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ods/iattribute.h>
#include <ods/itable.h>
namespace ods {
class IItem {
 public:
  IItem() = default;
  explicit IItem(const std::string& app_name);
  IItem(const std::string& app_name, const std::string& item_name);
  explicit IItem(int64_t app_id);
  virtual ~IItem() = default;

  [[nodiscard]] int64_t ApplicationId() const;
  void ApplicationId(int64_t ident);

  [[nodiscard]] int64_t ItemId() const;
  void ItemId(int64_t index);

  void Name(const std::string& item_name);
  [[nodiscard]] std::string Name() const;

  [[nodiscard]] uint64_t Created() const;
  [[nodiscard]] uint64_t Modified() const;

  [[nodiscard]] const std::string& ApplicationName() const;
  void ApplicationName(const std::string& name);

  void AppendAttribute(const IAttribute& attribute);

  template<typename T>
  void AppendAttribute(const ITable& table, bool base, const std::string& name, const T& value) {
    const auto *column = base ? table.GetColumnByBaseName(name) : table.GetColumnByName(name);
    if (column != nullptr && !column->DatabaseName().empty()) {
      AppendAttribute({column->ApplicationName(), column->BaseName(), value});
    }
  }

  void SetAttribute(const IAttribute& attribute);

  [[nodiscard]] bool ExistAttribute(const std::string& name) const;
  [[nodiscard]] bool ExistBaseAttribute(const std::string& base_name) const;

  [[nodiscard]] const IAttribute* GetAttribute(const std::string& name) const;
  [[nodiscard]] const IAttribute* GetBaseAttribute(const std::string& name) const;

  [[nodiscard]] const std::vector<IAttribute>& AttributeList() const;

  [[nodiscard]] std::vector<IAttribute>& AttributeList();

  template <typename T>
  T Value(const std::string& app_name) const {
    const auto* attr = GetAttribute(app_name);
    return attr == nullptr ? T {} : attr->Value<T>();
  }

  template <typename T>
  T BaseValue(const std::string& base_name) const {
    const auto* attr = GetBaseAttribute(base_name);
    return attr == nullptr ? T {} : attr->Value<T>();
  }
 private:
  int64_t     item_id_ = 0;        ///< Database index (Optional)
  std::string item_name_;          ///< Item name (Optional)
  int64_t     application_id_ = 0; ///< Table application ID
  std::string application_name_;   ///< Table name (Required if application ID is 0.
  std::vector<IAttribute> attribute_list_;
};

using ItemList = std::vector<std::unique_ptr<IItem>>;

}




