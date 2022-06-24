/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/wx.h>
#include "odsdocument.h"

namespace ods::gui {

class CommonPanel : public wxPanel {
 public:

  explicit CommonPanel(wxWindow* parent);
  ~CommonPanel() override;

  [[nodiscard]] OdsDocument* GetDocument() const;
  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;
  void Update() override;
 private:
  wxString name_;
  wxString description_;
  wxString version_;
  wxString created_;
  wxString created_by_;
  wxString base_version_;
  wxString modified_;
  wxString modified_by_;
  wxString source_name_;
  wxString source_type_;
  wxString source_info_;
  wxTimer  save_timer_;
  void OnSaveTimer(wxTimerEvent& event);

  wxDECLARE_EVENT_TABLE();
};

} // end namespace




