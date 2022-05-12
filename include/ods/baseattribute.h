/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <vector>
#include "ods/icolumn.h"
namespace ods {
class BaseAttribute : public IColumn {
 public:
  explicit BaseAttribute(const std::string& base_name);

  [[nodiscard]] bool Selected() const {
    return selected_;
  }
  void Selected(bool selected) {
    selected_ = selected;
  }

  [[nodiscard]] bool Mandatory() const {
    return mandatory_;
  }
  void Mandatory(bool mandatory) {
    mandatory_ = mandatory;
  }
 private:
  bool selected_ = false;
  bool mandatory_ = false;

};

[[nodiscard]] std::vector<BaseAttribute> GetBaseAttributeList(BaseId base_id);
[[nodiscard]] std::vector<std::string> GetParentBaseName(BaseId base_id);
}




