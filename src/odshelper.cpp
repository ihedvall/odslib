/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "odshelper.h"
#include <sstream>
#include <boost/beast/core/detail/base64.hpp>
#include <util/logstream.h>
#include <util/stringutil.h>
#include <util/timestamp.h>


using namespace util::log;
using namespace util::string;
using namespace util::time;
using namespace boost::beast::detail;

namespace {
  uint8_t ConvertCharacterToByte(uint8_t input) {
    switch (input) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return input - static_cast<uint8_t>('0');

      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        return input - static_cast<uint8_t>('A') + 0xA;

      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
        return input - static_cast<uint8_t>('a') + 0xA;

      default:
        break;

    }
    std::ostringstream err;
    err << "Invalid HEX input character. Character:  " << static_cast<char>(input);
    throw std::runtime_error(err.str());
  }
}

namespace ods {

std::string OdsHelper::ConvertToDumpString(const std::string& value) {
  std::ostringstream temp;
  for (const char cin : value) {
    switch (cin) {
      case '^':
        temp << "~ESC~";
        break;

      case '~':
        temp << "~TILDE~";
        break;

      case '\n':
        temp << "~LF~";
        break;

      case '\r':
        temp << "~CR~";
        break;

      default:
        temp << cin;
        break;
    }
  }
  return temp.str();
}

std::vector<std::string> OdsHelper::SplitDumpLine(const std::string& input_line) {
  std::vector<std::string> value_list;
  std::ostringstream temp;
  std::ostringstream escape;
  bool escape_sequence = false;
  for (const char cin : input_line) {
    // If it is an escape sequence
    if (escape_sequence) {
      if (cin == '~') {
        escape_sequence = false;
        const auto escape_string = escape.str();
        if (escape_string == "ESC") {
          temp << '^';
        } else if (escape_string == "TILDE") {
          temp << '~';
        } else if (escape_string == "CR") {
          temp << '\r';
        } else if (escape_string == "LF") {
          temp << '\n';
        } else if (escape_string == "NULL") {
          // Do nothing
        }
      } else {
        escape << cin;
      }
      continue;
    }

    switch (cin) {
      case '^': // Item delimiter
        value_list.emplace_back(Trim(temp.str()));
        temp.str("");
        temp.clear();
        break;

      case '\n':
      case '\r':
        // Skip any line ending characters
        break;

      case '~': // Step to next tilde
        escape_sequence = true;
        escape.str("");
        escape.clear();
        break;

      default:
        temp << cin;
        break;
    }
  }
  // Normally, any remaining string in 'temp.str()', should
  // be pushed onto the value_list but not in this case
  // as the last value must be delimited with a '^' character.
  return value_list;
}

std::vector<uint8_t> OdsHelper::FromBase64(const std::string& value) {
  std::vector<uint8_t> temp;
  if (value.empty()) {
    return temp;
  }

  try {
    const auto dest_size = base64::decoded_size(value.size());
    temp.resize(static_cast<size_t>(dest_size),0);

    const auto size_pair = base64::decode(temp.data(), value.data(), value.size());
    if (dest_size > size_pair.first) {
      temp.resize(size_pair.first);
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Base64 conversion error. Error: " << err.what();
    temp.clear();
  }
  return temp;
}

std::string OdsHelper::ToBase64(const std::vector<uint8_t> &byte_array) {
  std::string temp;
  if (byte_array.empty()) {
    return temp;
  }

  const auto dest_size = base64::encoded_size(byte_array.size());
  temp.resize(dest_size,'\0');
  const size_t actual_size = base64::encode(temp.data(), byte_array.data(), byte_array.size());
  if (actual_size != dest_size) {
    temp.resize(actual_size);
  }
  return temp;
}

bool OdsHelper::FetchDbtRow(const ITable &table, IItem &row, std::ifstream &file) {

  try {
    row.ApplicationId(table.ApplicationId());

    row.AttributeList().clear();
    if (file.eof()) {
      return false;
    }
    std::string input_line;
    std::getline(file,input_line);
    auto value_list = OdsHelper::SplitDumpLine(input_line);
    size_t column_index = 0;
    for (const IColumn& column : table.Columns()) {
      if (column.DatabaseName().empty()) {
        continue;
      }
      if (column_index >= value_list.size()) {
        break;
      }
      std::string& value = value_list[column_index];
      // Need to check some data type to check that they are in the right format.
      switch (column.DataType()) {
        case DataType::DtDate: {
          // The input may use the YYYY-MM-DD hh:mm:ss.xxx format while the internal format uses
          // the ODS YYYYMMDDhhmmssxxx format.
          // If the input string is YYYY-MM-DD, it is assumed that this is a local date time.
          // This format was used in older database dumps.
          const bool local_time = value.size() <= 10;
          const uint64_t ns1970 = IsoTimeToNs(value, local_time);
          value = NsToIsoTime(ns1970,3);
          break;
        }

        case DataType::DtBlob: // Note base64 coded format of a byte array
        default:
          // The DBT string format matches the internal format.
          break;
      }
      IAttribute attr(column.ApplicationName(), column.BaseName(), value.c_str() );
      row.AppendAttribute(attr);
      ++column_index;
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Invalid row. Error: " << err.what();
    return false;
  }
  return true;
}

std::string OdsHelper::ToHexString(const std::vector<uint8_t>& byte_array) {

  if (byte_array.empty()) {
    // Return an empty string so the caller can insert a NULL instead of the X'' string
    return {};
  }
  std::ostringstream temp;
  temp << "X'";
  for ( uint8_t input : byte_array) {
    temp <<  std::uppercase << std::setfill('0') << std::setw(2) << std::hex
                     << static_cast<uint16_t>(input);
  }
  temp << "'";
  return temp.str();
}

std::vector<uint8_t> OdsHelper::FromHexString(const std::string &hex) {
  // Assume the X'0102' format
  if (hex.empty()) {
    return {};
  }
  const bool x_format = hex[0] == 'X' || hex[0] == 'x';
  size_t hex_size = hex.size();
  if (x_format) {
    if (hex_size <= 3)  {
      return {};
    }
    hex_size -= 3;
  }
  hex_size /= 2;
  std::vector<uint8_t> temp;
  temp.reserve(hex_size);

  try {
    for (size_t index = (x_format ? 2 : 0); index < hex.size() - 1; index += 2) {
      const uint8_t high_byte = ConvertCharacterToByte(hex[index]);
      const uint8_t low_byte = ConvertCharacterToByte(hex[index + 1]);
      const uint8_t value = (high_byte * 0x10) + low_byte;
      temp.emplace_back(value);
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Invalid HEX string detected. Error: " << err.what();
  }
  return temp;
}

} // ods