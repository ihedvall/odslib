/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <wx/treelist.h>
#include <ods/iitem.h>
#include "meastreedata.h"

namespace ods::gui {
class MeasSelectionPanel : public wxPanel {
 public:
  explicit MeasSelectionPanel(wxWindow *parent);

  [[nodiscard]] ItemList& TestList() {
    return test_list_;
  }

  [[nodiscard]] ItemList& FileList() {
    return file_list_;
  }

  [[nodiscard]] ItemList& MeasList() {
    return meas_list_;
  }

  void RedrawTestList();
  void RedrawFileList();
  void RedrawMeasList();
 private:
  wxTreeListCtrl *left_ = nullptr;
  wxImageList image_list_;
  ItemList  test_list_;
  ItemList  file_list_;
  ItemList  meas_list_;
  void OnLeftExpanding(wxTreeListEvent& event);
  void OnSelectionChange(wxTreeListEvent& event);

  void RedrawFileList(wxTreeListItem& root_item, int64_t parent_test);
  void RedrawMeasList(wxTreeListItem& root_item, int64_t parent_file);
  wxDECLARE_EVENT_TABLE();
};

} // end namespace
