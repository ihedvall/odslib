/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <wx/wx.h>

namespace ods {

class GlobalNameApp : public wxApp {
 public:
  bool OnInit() override;
  int OnExit() override;

 private:
  std::string notepad_; ///< Path to notepad.exe if it exist

  void OpenFile(const std::string& filename) const;
  void OnOpenLogFile(wxCommandEvent& event);
  void OnUpdateOpenLogFile(wxUpdateUIEvent& event);

 wxDECLARE_EVENT_TABLE();
};

wxDECLARE_APP(GlobalNameApp);

} // ods

