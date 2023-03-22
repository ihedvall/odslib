/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <cstdint>
#include <string>
#include <sstream>
#include <typeinfo>
#include <vector>

#pragma once

namespace ods {

class IAttribute {
 private:
  std::string name_; ///< Application Name (Required)
  std::string base_name_; ///< Base name of the column (Optional but recommended)
  std::string value_; ///< Note that BLOB are stored as Base64 strings

 public:
  IAttribute() = default;
  virtual ~IAttribute() = default;

  IAttribute(const std::string &name, const char* value);
  IAttribute(const std::string &name, const std::string& base_name, const char* value);

  template <typename T>
  IAttribute(const std::string &name, const T& value)
    : name_(name) {
      Value(value);
  }

  template <typename T>
  IAttribute(const std::string &name, const std::string& base_name, const T& value)
    : name_(name),
      base_name_(base_name) {
    Value(value);
  }

  [[nodiscard]] const std::string& Name() const;
  void Name(const std::string& name);
  [[nodiscard]] const std::string& BaseName() const;
  void BaseName(const std::string& name);

  /** \brief Checks if the value is an unsigned number.
   *
   * Checks if a value is an unsigned number. Typically used to check
   * if a DtDate value was added as an ISO time string or as (uint64_t)
   * nano-seconds since 1970.
   * @return True if the value string is an unsigned number.
   */
  [[nodiscard]] bool IsValueUnsigned() const;

  [[nodiscard]] bool IsValueEmpty() const { ///< Returns true if the value is an empty string
    return value_.empty();
  }

  template <typename T>
  [[nodiscard]] T Value() const {
    T val {};
    std::istringstream temp(value_);
    try {
      temp >> val;
    } catch (const std::exception& ) {
    }
    return val;
  }

  template <typename T>
  void Value(const T& value) {
    try {
       value_ = std::to_string(value);
    } catch (const std::exception& ) {
      value_ = {};
    }
  }

};

template<>
std::string IAttribute::Value<std::string>() const;

template <>
[[nodiscard]] bool IAttribute::Value<bool>() const;

template <>
[[nodiscard]] std::vector<uint8_t> IAttribute::Value<std::vector<uint8_t>>() const;

template <>
void IAttribute::Value<std::string>(const std::string& value);

template <>
void IAttribute::Value<double>(const double& value);

template <>
void IAttribute::Value<float>(const float& value);

template <>
void IAttribute::Value<bool>(const bool& value);

template <>
void IAttribute::Value<std::vector<uint8_t>>(const std::vector<uint8_t>& value);
} // end namespace


