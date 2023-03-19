/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <algorithm>
#include <boost/beast/core/detail/base64.hpp>
#include <util/stringutil.h>
#include "ods/iattribute.h"

using namespace util::string;
using namespace boost::beast::detail;
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

bool IAttribute::IsValueUnsigned() const {
  if (value_.empty()) {
    return false;
  }
  return std::ranges::all_of(value_, [&] (const auto& input) {
    return ::isdigit(input);
  });
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

template<>
std::vector<uint8_t> IAttribute::Value<std::vector<uint8_t>>() const {
  std::vector<uint8_t> temp;
  if (value_.empty()) {
    return std::move(temp);
  }

  const auto dest_size = base64::decoded_size(value_.size());
  temp.resize(dest_size,0);
  const auto size_pair = base64::decode(temp.data(), value_.c_str(), dest_size);
  if (dest_size > size_pair.first) {
    temp.resize(size_pair.first);
  }
  return std::move(temp);
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

template <>
void IAttribute::Value<std::vector<uint8_t>>(const std::vector<uint8_t>& value) {
  if (value.empty()) {
    value_.clear();
    return;
  }
  const auto dest_size = base64::encoded_size(value.size());
  value_.resize(dest_size,'\0');
  base64::encode(value_.data(), value.data(), value.size());
}

} // end namespace