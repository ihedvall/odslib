/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <wx/wx.h>
#include <wx/docview.h>

namespace ods {

class GlobalNameDoc : public wxDocument {
 public:
  GlobalNameDoc();
  ~GlobalNameDoc() override = default;

 protected:
  bool OnNewDocument() override;

 private:
  void OnNewDatabase(wxCommandEvent& event);

 wxDECLARE_EVENT_TABLE();

  wxDECLARE_DYNAMIC_CLASS(GlobalNameDoc);
};

} // ods


