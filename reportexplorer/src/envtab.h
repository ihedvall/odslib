/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>

namespace ods::gui {

class EnvTab : public wxPanel {
 public:

  explicit EnvTab(wxWindow* parent);
  ~EnvTab() override = default;

  void Update() override;
  void OnEnvSelected(wxUpdateUIEvent &event);
  void OnEditEnv(wxCommandEvent& event);
  void OnDoubleClick(wxListEvent& event);
  void OnDeleteEnv(wxCommandEvent& event);
 private:
  void RedrawEnvList();
  wxListView* list = nullptr;
  void OnContextMenu(wxContextMenuEvent& event);
  wxDECLARE_EVENT_TABLE();
};

} // end namespace




