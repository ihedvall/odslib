/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "odsdocument.h"

namespace ods::gui {

class RelationPanel : public wxPanel {
 public:
  explicit RelationPanel(wxWindow* parent);

  [[nodiscard]] OdsDocument* GetDoc() const;

  void Update() override;
  void OnUpdateSingleRelationSelected(wxUpdateUIEvent &event);
  void OnUpdateRelationSelected(wxUpdateUIEvent &event);

  void OnAddRelation(wxCommandEvent &event);
  void OnEditRelation(wxCommandEvent&);
  void OnDeleteRelation(wxCommandEvent &event);

 private:
  wxListView* list_ = nullptr;
  wxImageList image_list_;

  void RedrawRelationList();

  void OnDoubleClickRelation(wxListEvent &event);
  void OnRightClick(wxContextMenuEvent& event);

  [[nodiscard]] IRelation* GetSelectedRelation();
  void SelectRelation(const std::string& name);

 wxDECLARE_EVENT_TABLE();
};

} // ods::gui


