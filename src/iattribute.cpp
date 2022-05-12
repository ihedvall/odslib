/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <util/stringutil.h>
#include "ods/iattribute.h"

using namespace util::string;
namespace ods {

IAttribute::IAttribute(const std::string &name, const char* value)
: name_(name),
  value_(std::string(value != nullptr ? value : "")) {
}

IAttribute::IAttribute(const std::string &name, const std::string& base_name, const char* value)
: name_(name),
  base_name_(base_name),
  value_(std::string(value != nullptr ? value : "")) {
}

const std::string &IAttribute::BaseName() const {
  return base_name_;
}
void IAttribute::BaseName(const std::string &name) {
  base_name_ = name;
}

const std::string &IAttribute::Name() const {
  return name_;
}
void IAttribute::Name(const std::string &name) {
  name_ = name;
}

template<>
std::string IAttribute::Value<std::string>() const {
  return value_;
}

template <>
[[nodiscard]] bool IAttribute::Value<bool>() const {
  if (value_.empty()) {
    return false;
  }
  switch (value_[0]) {
    case '1':
    case 't':
    case 'T':
    case 'y':
    case 'Y':
      return true;
    default:
      break;
  }
  return false;
}

template <>
void IAttribute::Value<std::string>(const std::string& value) {
  value_ = value;
}

template <>
void IAttribute::Value<double>(const double& value) {
  value_ = DoubleToString(value);;
}

template <>
void IAttribute::Value<float>(const float& value) {
  value_ = FloatToString(value);;
}

template <>
void IAttribute::Value<bool>(const bool& value) {
  value_ = value ? "1" : "0";
}


}