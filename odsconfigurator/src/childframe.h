/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <wx/wx.h>
#include <wx/docmdi.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include "odsdocument.h"

namespace ods::gui {
class ChildFrame : public wxDocMDIChildFrame {
 public:
  ChildFrame(wxDocument *doc,
            wxView *view,
            wxMDIParentFrame *parent,
            wxWindowID id,
            const wxString& title);
  ChildFrame() = default;
  void Update() override;

 private:
  wxImageList image_list_;
  wxNotebook* notebook_ = nullptr;
  [[nodiscard]] OdsDocument* GetDoc() const;
  void OnUpdateSave(wxUpdateUIEvent &event);
  void OnSave(wxCommandEvent&);
  void OnSaveAs(wxCommandEvent&);
  void OnCloseDoc(wxCommandEvent&);
  void OnPageChange(wxBookCtrlEvent& event);

  void OnUpdateTable(wxUpdateUIEvent &event);
  void OnUpdateTableSelected(wxUpdateUIEvent &event);
  void OnAddTable(wxCommandEvent& event);
  void OnEditTable(wxCommandEvent& event);
  void OnDeleteTable(wxCommandEvent& event);

  void OnUpdateSingleColumnSelected(wxUpdateUIEvent &event);
  void OnUpdateColumnSelected(wxUpdateUIEvent &event);
  void OnAddColumn(wxCommandEvent& event);
  void OnEditColumn(wxCommandEvent& event);
  void OnDeleteColumn(wxCommandEvent& event);

  void OnUpdateEnum(wxUpdateUIEvent& event);
  void OnUpdateSingleEnumSelected(wxUpdateUIEvent& event);
  void OnUpdateEnumSelected(wxUpdateUIEvent& event);
  void OnAddEnum(wxCommandEvent& event);
  void OnEditEnum(wxCommandEvent& event);
  void OnDeleteEnum(wxCommandEvent& event);

  void OnUpdateSingleEnumItemSelected(wxUpdateUIEvent& event);
  void OnUpdateEnumItemSelected(wxUpdateUIEvent& event);
  void OnAddEnumItem(wxCommandEvent& event);
  void OnEditEnumItem(wxCommandEvent& event);
  void OnDeleteEnumItem(wxCommandEvent& event);

  wxDECLARE_EVENT_TABLE();

};
}




