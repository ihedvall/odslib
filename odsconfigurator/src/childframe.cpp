/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <sstream>
#include <filesystem>
#include <wx/richmsgdlg.h>
#include "childframe.h"
#include "odsconfigid.h"
#include "commonpanel.h"
#include "tablepanel.h"
#include "enumpanel.h"
#include "tabledialog.h"
namespace {

// Defines the page number in the notebook.
constexpr int kCommonPanel = 0;
constexpr int kTablePanel = 1;
constexpr int kEnumPanel = 2;

} // Empty namespace

namespace ods::gui {

wxBEGIN_EVENT_TABLE(ChildFrame, wxDocMDIChildFrame) // NOLINT(cert-err58-cpp)
  EVT_UPDATE_UI(wxID_SAVE, ChildFrame::OnUpdateSave)
  EVT_BUTTON(wxID_SAVE, ChildFrame::OnSave)
  EVT_BUTTON(wxID_SAVEAS, ChildFrame::OnSaveAs)
  EVT_BUTTON(wxID_CLOSE, ChildFrame::OnCloseDoc)
  EVT_NOTEBOOK_PAGE_CHANGED(kIdNotebook, ChildFrame::OnPageChange)

  EVT_UPDATE_UI(kIdAddTable, ChildFrame::OnUpdateTable)
  EVT_UPDATE_UI(kIdEditTable, ChildFrame::OnUpdateTableSelected)
  EVT_UPDATE_UI(kIdDeleteTable, ChildFrame::OnUpdateTableSelected)
  EVT_UPDATE_UI(kIdCopyTable, ChildFrame::OnUpdateTableSelected)
  EVT_MENU(kIdAddTable, ChildFrame::OnAddTable)
  EVT_MENU(kIdEditTable, ChildFrame::OnEditTable)
  EVT_MENU(kIdDeleteTable, ChildFrame::OnDeleteTable)

  EVT_UPDATE_UI(kIdAddColumn, ChildFrame::OnUpdateTableSelected)
  EVT_UPDATE_UI(kIdEditColumn, ChildFrame::OnUpdateSingleColumnSelected)
  EVT_UPDATE_UI(kIdDeleteColumn, ChildFrame::OnUpdateColumnSelected)
  EVT_MENU(kIdAddColumn, ChildFrame::OnAddColumn)
  EVT_MENU(kIdEditColumn, ChildFrame::OnEditColumn)
  EVT_MENU(kIdDeleteColumn, ChildFrame::OnDeleteColumn)

  EVT_UPDATE_UI(kIdAddEnum, ChildFrame::OnUpdateEnum)
  EVT_UPDATE_UI(kIdEditEnum, ChildFrame::OnUpdateSingleEnumSelected)
  EVT_UPDATE_UI(kIdDeleteEnum, ChildFrame::OnUpdateEnumSelected)
  EVT_MENU(kIdAddEnum, ChildFrame::OnAddEnum)
  EVT_MENU(kIdEditEnum, ChildFrame::OnEditEnum)
  EVT_MENU(kIdDeleteEnum, ChildFrame::OnDeleteEnum)

  EVT_UPDATE_UI(kIdAddEnumItem, ChildFrame::OnUpdateSingleEnumSelected)
  EVT_UPDATE_UI(kIdEditEnumItem, ChildFrame::OnUpdateSingleEnumItemSelected)
  EVT_UPDATE_UI(kIdDeleteEnumItem, ChildFrame::OnUpdateEnumItemSelected)
  EVT_MENU(kIdAddEnumItem, ChildFrame::OnAddEnumItem)
  EVT_MENU(kIdEditEnumItem, ChildFrame::OnEditEnumItem)
  EVT_MENU(kIdDeleteEnumItem, ChildFrame::OnDeleteEnumItem)

wxEND_EVENT_TABLE()

ChildFrame::ChildFrame(wxDocument *doc,
                     wxView *view,
                     wxMDIParentFrame *parent,
                     wxWindowID id,
                     const wxString& title)
    : wxDocMDIChildFrame(doc, view, parent, id, title, wxDefaultPosition, wxDefaultSize,
                         wxDEFAULT_FRAME_STYLE, wxASCII_STR(wxFrameNameStr)),
                         image_list_(32,32,false,8) {
  SetIcon(wxIcon("SUB_ICON", wxBITMAP_TYPE_ICO_RESOURCE));

  image_list_.Add(wxBitmap("NOTEBOOK_LIST", wxBITMAP_TYPE_BMP_RESOURCE));
  notebook_ = new wxNotebook(this, kIdNotebook);
  notebook_->SetImageList(&image_list_);

  auto* common = new CommonPanel(notebook_);
  auto* config = new TablePanel(notebook_);
  auto* enumerate = new EnumPanel(notebook_);
  //auto* relation = new CommonPanel(notebook_);
  notebook_->AddPage(common, L"General", false,4);
  notebook_->AddPage(config, L"Database Design", false,5);
  notebook_->AddPage(enumerate, L"Enumerations", false,6);

  auto* save_button = new wxButton(this, wxID_SAVE, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto* save_as_button = new wxButton(this, wxID_SAVEAS, wxGetStockLabel(wxID_SAVEAS, wxSTOCK_FOR_BUTTON));
  auto* cancel_button = new wxButton(this, wxID_CLOSE, wxGetStockLabel(wxID_CLOSE, wxSTOCK_FOR_BUTTON));

  auto* system_sizer = new wxBoxSizer(wxHORIZONTAL);
  system_sizer->Add(save_button, 0, wxALIGN_LEFT | wxALL, 10);
  system_sizer->Add(save_as_button, 0, wxALIGN_LEFT | wxALL, 10);
  system_sizer->Add(cancel_button, 0, wxALIGN_LEFT | wxALL, 10);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(notebook_, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  save_button->SetDefault();

  notebook_->SetSelection(1);
  notebook_->SetPageImage(1,1);

}

void ChildFrame::OnUpdateSave(wxUpdateUIEvent &event) {
  const auto* doc = GetDocument();
  if (doc == nullptr) {
    event.Enable(false);
    return;
  }

  try {
    std::filesystem::path file(doc->GetFilename().ToStdString());
    if (!std::filesystem::exists(file)) {
      event.Enable(false);
      return;
    }
  } catch (const std::exception& ) {
    event.Enable(false);
    return;
  }
  event.Enable(true);
}

void ChildFrame::OnSave(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc != nullptr) {
    doc->Save();
  }
}

void ChildFrame::OnSaveAs(wxCommandEvent&) {
  auto* doc = GetDocument();
  if (doc != nullptr) {
    doc->SaveAs();
  }
}

void ChildFrame::OnCloseDoc(wxCommandEvent& event) {
  auto *doc = GetDocument();
  if (doc != nullptr) {
    doc->DeleteAllViews();
  }
}

void ChildFrame::Update() {
  const auto* doc = GetDoc();
  if (doc != nullptr && !doc->GetTitle().empty() && GetTitle() != doc->GetTitle()) {
    SetTitle(doc->GetTitle());
  }
  for (size_t tab = 0; tab < notebook_->GetPageCount(); ++tab) {
    auto* page = notebook_->GetPage(tab);
    if (page != nullptr) {
      page->Update();
    }
  }
  wxDocMDIChildFrame::Update();
}

void ChildFrame::OnPageChange(wxBookCtrlEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }
  const auto old_page = event.GetOldSelection();
  const auto page = event.GetSelection();
  auto* window = notebook_->GetPage(page);
  if (old_page != wxNOT_FOUND) {
    notebook_->SetPageImage(old_page, old_page + 4);
  }
  if (page != wxNOT_FOUND) {
    notebook_->SetPageImage(page, page);
  }
  if (page != kCommonPanel && window != nullptr) {
    window->Update();
  }

}

void ChildFrame::OnUpdateTable(wxUpdateUIEvent &event) {
  event.Enable(notebook_ != nullptr && notebook_->GetSelection() == kTablePanel);
}

void ChildFrame::OnUpdateTableSelected(wxUpdateUIEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr || notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  if (notebook_->GetSelection() != kTablePanel) {
    event.Enable(false);
    return;
  }

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    event.Enable(false);
    return;
  }
  table_panel->OnUpdateTableSelected(event);
}

void ChildFrame::OnAddTable(wxCommandEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr) {
    return;
  }
  auto& model = doc->GetModel();
  ITable empty;
  auto* selected = doc->GetSelectedTable();
  if (selected != nullptr) {
    empty.ParentId(selected->ApplicationId());
    empty.BaseId(selected->BaseId());
  }
  empty.ApplicationId(model.FindNextTableId(empty.ParentId()));

  TableDialog dialog(this, model, empty);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  const auto name = dialog.GetTable().ApplicationName();
  model.AddTable(dialog.GetTable());

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    return;
  }
  table_panel->RedrawTableList();
  table_panel->SelectTable(dialog.GetTable().ApplicationName());
}

void ChildFrame::OnEditTable(wxCommandEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr) {
    return;
  }
  auto* selected = doc->GetSelectedTable();
  if (selected == nullptr) {
    return;
  }

  TableDialog dialog(this, doc->GetModel(), *selected);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  *selected = dialog.GetTable();
  Update();
}

void ChildFrame::OnDeleteTable(wxCommandEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr) {
    return;
  }
  const auto* selected = doc->GetSelectedTable();
  if (selected == nullptr) {
    return;
  }
  std::ostringstream ask;
  ask << "Do you want to delete the table?" << std::endl
      << "Table Name: " << selected->ApplicationName();
  int ret = wxMessageBox(ask.str(), "Delete Table Dialog",
                         wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION,
                         this);

  if (ret != wxYES) {
    return;
  }
  auto& model = doc->GetModel();
  model.DeleteTable(selected->ApplicationId());
  Update();
}

OdsDocument *ChildFrame::GetDoc() const {
  return wxDynamicCast(GetDocument(), OdsDocument); //NOLINT
}

void ChildFrame::OnUpdateColumnSelected(wxUpdateUIEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr || notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  const ITable* table = doc->GetSelectedTable();
  if (notebook_->GetSelection() != kTablePanel || table == nullptr) {
    event.Enable(false);
    return;
  }

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    event.Enable(false);
    return;
  }
  return table_panel->OnUpdateColumnSelected(event);
}

void ChildFrame::OnUpdateSingleColumnSelected(wxUpdateUIEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr || notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  const ITable* table = doc->GetSelectedTable();
  if (notebook_->GetSelection() != kTablePanel || table == nullptr) {
    event.Enable(false);
    return;
  }

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    event.Enable(false);
    return;
  }
  return table_panel->OnUpdateSingleColumnSelected(event);
}

void ChildFrame::OnAddColumn(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    return;
  }
  return table_panel->OnAddColumn(event);
}

void ChildFrame::OnEditColumn(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    return;
  }
  return table_panel->OnEditColumn(event);
}
void ChildFrame::OnDeleteColumn(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* table_panel = dynamic_cast<TablePanel*>(notebook_->GetCurrentPage());
  if (table_panel == nullptr) {
    return;
  }
  return table_panel->OnDeleteColumn(event);
}

void ChildFrame::OnUpdateEnum(wxUpdateUIEvent &event) {
  if (notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(notebook_->GetSelection() == kEnumPanel);
}

void ChildFrame::OnUpdateSingleEnumSelected(wxUpdateUIEvent &event) {
  if (notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  if (notebook_->GetSelection() != kEnumPanel) {
    event.Enable(false);
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    event.Enable(false);
    return;
  }
  return enum_panel->OnUpdateSingleEnumSelected(event);
}

void ChildFrame::OnUpdateEnumSelected(wxUpdateUIEvent &event) {
  if (notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  if (notebook_->GetSelection() != kEnumPanel) {
    event.Enable(false);
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    event.Enable(false);
    return;
  }
  return enum_panel->OnUpdateEnumSelected(event);
}

void ChildFrame::OnAddEnum(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    return;
  }
  return enum_panel->OnAddEnum(event);
}

void ChildFrame::OnEditEnum(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    return;
  }
  return enum_panel->OnEditEnum(event);
}

void ChildFrame::OnDeleteEnum(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    return;
  }
  return enum_panel->OnDeleteEnum(event);
}

void ChildFrame::OnUpdateSingleEnumItemSelected(wxUpdateUIEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr || notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  if (notebook_->GetSelection() != kEnumPanel) {
    event.Enable(false);
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    event.Enable(false);
    return;
  }
  return enum_panel->OnUpdateSingleEnumItemSelected(event);
}

void ChildFrame::OnUpdateEnumItemSelected(wxUpdateUIEvent &event) {
  auto* doc = GetDoc();
  if (doc == nullptr || notebook_ == nullptr) {
    event.Enable(false);
    return;
  }
  if (notebook_->GetSelection() != kEnumPanel) {
    event.Enable(false);
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    event.Enable(false);
    return;
  }
  return enum_panel->OnUpdateEnumItemSelected(event);
}

void ChildFrame::OnAddEnumItem(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    return;
  }
  return enum_panel->OnAddEnumItem(event);
}

void ChildFrame::OnEditEnumItem(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    return;
  }
  return enum_panel->OnEditEnumItem(event);
}

void ChildFrame::OnDeleteEnumItem(wxCommandEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }

  auto* enum_panel = dynamic_cast<EnumPanel*>(notebook_->GetCurrentPage());
  if (enum_panel == nullptr) {
    return;
  }
  return enum_panel->OnDeleteEnumItem(event);
}


} // end namespace



