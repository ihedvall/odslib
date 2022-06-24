/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>

namespace ods::gui {
class EnumItemDialog : public wxDialog {
 public:
  int64_t key_ = 0;
  wxString value_;

  EnumItemDialog(wxWindow *parent);

};

} // end namespace




