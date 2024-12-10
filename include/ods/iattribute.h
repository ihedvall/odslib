/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <cstdint>
#include <string>
#include <sstream>
#include <typeinfo>
#include <utility>
#include <vector>

#pragma once

namespace ods {

/** \brief Generic class for a column value in a database
 *
 * The class is used to represent a column value in a generic way.
 * The internal storage is a string. The interpretation of the string
 * so the different data types are stored as below. Note that the
 * std::locale("C") is used when formatting integers and float values.
 * <ul>
 * <li> DtString: As an UTF8 coded string.
 * <li> DtExternalRef: As an UTF8 coded string.
 * <li> DShort: Integer (64-bit) as a string.
 * <li> DtByte: Integer (64-bit) as a string.
 * <li> DtLong: Integer (64-bit) as a string.
 * <li> DtLongLong: Integer (64-bit) as a string.
 * <li> DtId: Integer (64-bit) as string.
 * <li> DtEnum: Integer (64-bit) as a string.
 * <li> DtFloat: Float as text. Note uses C-locale decimal point (dot not comma).
 * <li> DtDouble: Double as text. Note uses C-locale decimal point (dot not comma).
 * <li> DtBoolean: String 0 as false or 1 as false.
 * <li> DtDate: ISO 8601 string (YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ).
 * <li> DtByteString: Byte array stored as a Base64 string.
 * <li> DtByte: As DtShort.
 * </ul>
 */
class IAttribute {
 private:
  std::string name_; ///< Application Name (Required)
  std::string base_name_; ///< Base name of the column (Optional but recommended)
  std::string value_; ///< Note that BLOB are stored as Base64 strings

 public:

  IAttribute() = default; ///< Default constructor
  virtual ~IAttribute() = default; ///< Default destructor

  IAttribute(std::string name, const char* value);
  IAttribute(std::string name, std::string base_name, const char* value);

  template <typename T>
  IAttribute(std::string name, const T& value)
    : name_(std::move(name)) {
      Value(value);
  }

  template <typename T>
  IAttribute(std::string name, std::string  base_name, const T& value)
    : name_(std::move(name)),
      base_name_(std::move(base_name)) {
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

  /** \brief Checks if the internal string is empty.
   *
   * The function returns true if the internal string value is empty.
   * This can indicate that the value is invalid or not updated if
   * the value is a number or a timestamp (DtDate).
   * @return True if the value is empty.
   */
  [[nodiscard]] bool IsValueEmpty() const { ///< Returns true if the value is an empty string
    return value_.empty();
  }

  template <typename T>
  [[nodiscard]] T Value() const {
    T val {};

    try {
      std::istringstream temp(value_);
      temp >> val;
    } catch (const std::exception& ) {
    }
    return val;
  }

  template <typename T>
  void Value(T value) {
    try {
       value_ = std::to_string(value);
    } catch (const std::exception& ) {
      value_ = {};
    }
  }

};

template<>
std::string IAttribute::Value() const;

template <>
[[nodiscard]] bool IAttribute::Value() const;

template <>
[[nodiscard]] std::vector<uint8_t> IAttribute::Value() const;

template <>
[[nodiscard]] float IAttribute::Value() const;

template <>
[[nodiscard]] double IAttribute::Value() const;


template <>
void IAttribute::Value(std::string value);

template <>
void IAttribute::Value(double value);

template <>
void IAttribute::Value(float value);

template <>
void IAttribute::Value(bool value);

template <>
void IAttribute::Value(std::vector<uint8_t> value);

template <>
void IAttribute::Value(const char* value);
} // end namespace


