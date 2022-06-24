/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "appnamevalidator.h"

namespace ods::gui {

AppNameValidator::AppNameValidator(wxString *value) :
    wxTextValidator(wxFILTER_ALPHANUMERIC | wxFILTER_EMPTY, value) {
  SetCharIncludes("_");
}

wxObject *AppNameValidator::Clone() const {
  return new AppNameValidator(*this);
}

} // end namespace