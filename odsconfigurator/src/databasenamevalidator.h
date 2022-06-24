/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <sstream>
#include <wx/valtext.h>
#include <ods/idatabase.h>
namespace ods::gui {

class DatabaseNameValidator : public wxTextValidator {
 public:
  explicit DatabaseNameValidator(wxString* value);

  DatabaseNameValidator(const DatabaseNameValidator&) = default;

  [[nodiscard]] wxObject *Clone() const override;
  [[nodiscard]] wxString IsValid (const wxString &val) const override;
};



}


