/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <algorithm>
#include <utility>
#include <charconv>

#include "ods/iattribute.h"

#include "odshelper.h"

namespace ods {

IAttribute::IAttribute(std::string name, const char* value)
: name_(std::move(name)),
  value_(value != nullptr ? value : "") {
}

IAttribute::IAttribute(std::string name, std::string  base_name, const char* value)
: name_(std::move(name)),
  base_name_(std::move(base_name)),
  value_(value != nullptr ? value : "") {
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

bool IAttribute::IsValueUnsigned() const {
  if (value_.empty()) {
    return false;
  }
  // Must be all number
  return std::ranges::all_of(value_, [&] (const char& input) ->bool {
    return ::isdigit(input);
  });
}

template<>
std::string IAttribute::Value() const {
  return value_;
}

template <>
[[nodiscard]] bool IAttribute::Value() const {
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

template<>
std::vector<uint8_t> IAttribute::Value() const {
  return OdsHelper::FromBase64(value_);
}

template<>
float IAttribute::Value() const {
  if (value_.empty()) {
    return 0.0F;
  }
  float value = 0.0F;
  std::from_chars(value_.data(), value_.data() + value_.size(), value);
  return value;
}

template<>
double IAttribute::Value() const {
  if (value_.empty()) {
    return 0.0;
  }
  double value = 0.0;
  std::from_chars(value_.data(), value_.data() + value_.size(), value);
  return value;
}

template <>
void IAttribute::Value(std::string value) {
  value_ = std::move(value);
}

template <>
void IAttribute::Value(double value) {
  char temp[80] = {'\0'};
  std::to_chars(temp, temp + 78, value );
  value_ = temp;
}

template <>
void IAttribute::Value(float value) {
  char temp[80] = {'\0'};
  std::to_chars(temp, temp + 78, value );
  value_ = temp;
}

template <>
void IAttribute::Value(bool value) {
  value_ = value ? "1" : "0";
}

template <>
void IAttribute::Value(std::vector<uint8_t> value) {
  const auto temp = std::move(value);
  value_ = OdsHelper::ToBase64(temp);
}

template <>
void IAttribute::Value(const char* value) {
  value_ = value != nullptr ? value : "";
}
} // end namespace