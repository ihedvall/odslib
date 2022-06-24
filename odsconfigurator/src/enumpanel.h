/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "odsdocument.h"
namespace ods::gui {

class EnumPanel : public wxPanel {
 public:
  explicit EnumPanel(wxWindow* parent);

  [[nodiscard]] OdsDocument* GetDocument() const;

  void Update() override;
  void OnUpdateSingleEnumSelected(wxUpdateUIEvent &event);
  void OnUpdateEnumSelected(wxUpdateUIEvent &event);

  void OnAddEnum(wxCommandEvent &event);
  void OnEditEnum(wxCommandEvent&);
  void OnDeleteEnum(wxCommandEvent &event);

  void OnUpdateSingleEnumItemSelected(wxUpdateUIEvent &event);
  void OnUpdateEnumItemSelected(wxUpdateUIEvent &event);

  void OnAddEnumItem(wxCommandEvent &event);
  void OnEditEnumItem(wxCommandEvent &);
  void OnDeleteEnumItem(wxCommandEvent &event);
 private:
  wxListView* left_ = nullptr;
  wxListView* right_ = nullptr;
  wxImageList image_list_;

  void RedrawEnumList();
  void RedrawItemList();

  void OnEnumSelect(wxListEvent& event);
  void OnDoubleClickEnum(wxListEvent &event);
  void OnDoubleClickItem(wxListEvent &event);
  void OnRightClick(wxContextMenuEvent& event);

  void OnUpdateCopyEnumExist(wxUpdateUIEvent& event);
  void OnCopyEnum(wxCommandEvent& event);
  void OnPasteEnum(wxCommandEvent& event);

  [[nodiscard]] IEnum* GetSelectedEnum() const;
  bool GetSelectedItem(int64_t& key, std::string& value) const;
  void SelectEnum(const std::string& name);

 wxDECLARE_EVENT_TABLE();
};

}





