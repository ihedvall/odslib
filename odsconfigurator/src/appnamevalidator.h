/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <sstream>
#include <wx/valtext.h>
#include <ods/idatabase.h>
namespace ods::gui {

class AppNameValidator : public wxTextValidator {
 public:
  explicit AppNameValidator(wxString *value);

  AppNameValidator(const AppNameValidator &) = default;

  [[nodiscard]] wxObject *Clone() const override;

};

} // end namespace



