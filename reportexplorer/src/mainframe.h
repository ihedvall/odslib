/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/statusbr.h>

namespace ods::gui {
class MainFrame : public wxFrame {
 public:
  MainFrame(const wxString& title, const wxPoint& start_pos, const wxSize& start_size, bool maximized);
  void Update() override;
  void RedrawSelectedList();
  [[nodiscard]] size_t GetSelectedCount() const;
  void RemoveSelected();
 private:
  wxImageList image_list_;
  wxNotebook* notebook_ = nullptr;


  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnPageChange(wxBookCtrlEvent& event);

  void OnNewEnvTestDir(wxCommandEvent& event);
  void OnEnvSelected(wxUpdateUIEvent &event);
  void OnEditEnv(wxCommandEvent& event);
  void OnDeleteEnv(wxCommandEvent& event);
 wxDECLARE_EVENT_TABLE();
};
}

