/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "wx/docview.h"
#include <wx/ribbon/bar.h>

namespace ods {

class MainFrame : public wxDocParentFrame {
 public:
  MainFrame(const wxString& title, const wxPoint& start_pos, const wxSize& start_size, bool maximized);

 private:
  wxRibbonBar* ribbon_bar_ = nullptr;
  void MakeMenu();
  void MakeStatusBar();

  void OnClose(wxCloseEvent& event);
  void OnAbout(wxCommandEvent& event);
 wxDECLARE_EVENT_TABLE();
};

} // ods


