/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <cstdint>
#include <string>

namespace ods {
class IRelation {
 public:
  IRelation() = default;
  virtual ~IRelation() = default;
 private:
  int64_t application_id1_ = 0;
  int64_t application_id2_ = 0;

  std::string name_;
  std::string database_name_;
  std::string inverse_name_;
  std::string base_name_;
  std::string inverse_base_name_;

};

} // end namespace




