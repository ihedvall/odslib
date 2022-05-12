/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <cstdint>
#include <string>
#include <map>
namespace ods {
class IEnum {
 public:
  IEnum() = default;
  virtual ~IEnum() = default;
  bool operator == (const IEnum& enumerate) const = default;

  using ItemList =  std::map<int64_t, std::string>;

  [[nodiscard]] int64_t EnumId() const {
    return enum_id_;
  }
  void EnumId(int64_t enum_id) {
    enum_id_ = enum_id;
  }

  [[nodiscard]] const std::string& EnumName() const {
    return enum_name_;
  }
  void EnumName(const std::string& name) {
    enum_name_ = name;
  }

  [[nodiscard]] bool Locked() const {
    return locked_;
  }
  void Locked(bool locked) {
    locked_ = locked;
  }

  void AddItem(int64_t index, const std::string &item);
  void DeleteItem(int64_t index);

  [[nodiscard]] const ItemList& Items() const {
    return item_list_;
  }
  [[nodiscard]] int64_t GetNextKey() const {
    return item_list_.empty() ? 0 : item_list_.crbegin()->first + 1;
  }

 private:
  int64_t enum_id_ = 0;
  std::string enum_name_;
  bool locked_ = false;

  ItemList item_list_;
};

} // end namespace





