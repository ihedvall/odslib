/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <string_view>
#include <wx/wx.h>
#include <wx/treelist.h>
#include <wx/srchctrl.h>
#include <wx/filepicker.h>

#include "selectedcomparator.h"

namespace ods::gui {

class PlotTab : public wxPanel {
 public:
  explicit PlotTab(wxWindow* parent);

  void Update() override;
  void RedrawList();
 protected:
  void RedrawScript();
 private:
  wxTreeListCtrl* list_ = nullptr;
  wxBitmap up_image_;
  wxBitmap down_image_;
  wxSearchCtrl* channel_name_ = nullptr;
  wxComboBox* unit_ = nullptr;
  SelectedComparator comparator_;
  wxDirPickerCtrl* report_dir_ = nullptr;
  wxComboBox* x_axis_ = nullptr;
  wxTextCtrl* script_ = nullptr;

  wxString report_name_;

  void OnSelectXAxis(wxCommandEvent& event);
  void OnContextMenu(wxTreeListEvent& event);
  void OnListNotEmpty(wxUpdateUIEvent& event);
  void OnSelectX(wxCommandEvent& event);
  void OnSelectY1(wxCommandEvent& event);
  void OnSelectY2(wxCommandEvent& event);
  void OnViewOnly(wxCommandEvent& event);
 wxDECLARE_EVENT_TABLE();
};

} // end namespace
