/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "databasenamevalidator.h"

namespace ods::gui {

DatabaseNameValidator::DatabaseNameValidator(wxString *value) :
    wxTextValidator(wxFILTER_ALPHANUMERIC, value) {
  SetCharIncludes("_");
}

wxObject *DatabaseNameValidator::Clone() const {
  return new DatabaseNameValidator(*this);
}

wxString DatabaseNameValidator::IsValid(const wxString &val) const {
  const auto err = wxTextValidator::IsValid(val);
  if (!err.empty()) {
    return err;
  }
  const auto reserved = IsSqlReservedWord(val.ToStdString());
  if (reserved) {
    std::ostringstream error;
    error << "The name is a reserved SQL word.\nThis will cause problem when using SQL later on.\nName: "
          << val.ToStdString();
    return error.str();
  }
  return "";
}

}