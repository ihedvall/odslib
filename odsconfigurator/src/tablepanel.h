/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/treelist.h>
#include "odsdocument.h"
namespace ods::gui {

class TablePanel : public wxPanel {
 public:
  explicit TablePanel(wxWindow* parent);

  [[nodiscard]] OdsDocument* GetDocument() const;

  void Update() override;

  void OnUpdateTableSelected(wxUpdateUIEvent& event);

  void OnUpdateColumnSelected(wxUpdateUIEvent &event);
  void OnUpdateSingleColumnSelected(wxUpdateUIEvent &event);

  void OnAddColumn(wxCommandEvent &event);
  void OnEditColumn(wxCommandEvent &);
  void OnDeleteColumn(wxCommandEvent &event);

  void RedrawTableList();
  void RedrawColumnList();
  void SelectTable(const std::string& name);

 private:
  wxTreeListCtrl* left_ = nullptr;
  wxListView* right_ = nullptr;
  wxImageList image_list_;
  wxStaticText* table_info_ = nullptr;
  wxBitmap up_image_;
  wxBitmap down_image_;

  void OnTableSelect(wxTreeListEvent& event);
  void OnTableActivated(wxTreeListEvent& event);
  void OnTableRightClick(wxTreeListEvent& event);
  void OnColumnRightClick(wxContextMenuEvent& event);
  void OnColumnActivated(wxListEvent& );
  void OnUniqueFlag(wxCommandEvent& );
  void OnNoUniqueFlag(wxCommandEvent& );
  void OnIndexFlag(wxCommandEvent& );
  void OnNoIndexFlag(wxCommandEvent& );
  void OnColumnUp(wxCommandEvent& );
  void OnColumnDown(wxCommandEvent& );
  void OnSizeChange(wxSizeEvent& event);

  void OnUpdateCopyTableExist(wxUpdateUIEvent& event);
  void OnCopyTable(wxCommandEvent& event);
  void OnPasteTable(wxCommandEvent& event);
  IColumn* GetSelectedColumn();
 wxDECLARE_EVENT_TABLE();
};

}




