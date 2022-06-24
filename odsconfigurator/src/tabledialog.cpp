/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/valnum.h>
#include <wx/valtext.h>
#include <wx/valgen.h>
#include <wx/msgdlg.h>
#include <wx/grid.h>
#include <ods/odsdef.h>
#include "tabledialog.h"
#include "odsconfigid.h"
#include "databasenamevalidator.h"
#include "appnamevalidator.h"
namespace {

wxArrayString MakeBaseList() {
    wxArrayString list;
    for (int base_id = 0; base_id < 255; ++base_id) {
      const auto base = ods::BaseIdToUserText(static_cast<ods::BaseId>(base_id));
      if (base.empty()) {
        continue;
      }
      list.Add(base);
    }
    list.Sort();
    return list;
  }

wxArrayString MakeTableList(const ods::IModel& model) {
  const auto& table_list = model.AllTables();
  wxArrayString list;
  list.Add("");
  for (const auto* table : table_list) {
    if (table == nullptr || table->ApplicationName().empty()) {
      continue;
    }
    list.Add(table->ApplicationName());
  }
  list.Sort();
  return list;
}

} // end namespace

namespace ods::gui {

wxBEGIN_EVENT_TABLE(TableDialog, wxDialog) //NOLINT
  EVT_TREELIST_ITEM_CHECKED(kIdBaseList, TableDialog::OnToggleSelect)

  EVT_CHOICE(kIdBaseType, TableDialog::OnBaseChange)
  EVT_CHOICE(kIdParentTable, TableDialog::OnParentChange)
wxEND_EVENT_TABLE()

TableDialog::TableDialog(wxWindow *parent, const IModel& model, const ITable& original)
: wxDialog(parent, wxID_ANY, L"Table Dialog", wxDefaultPosition, wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
  model_(model),
  table_(original),
  image_list_(16, 16, false, 2) {
  image_list_.Add(wxBitmap("BASE_LIST", wxBITMAP_TYPE_BMP_RESOURCE));
  const bool lock_id = table_.ApplicationId() > 0;
  if (table_.ApplicationId() == 0) {
    table_.ApplicationId(model_.FindNextTableId(table_.ParentId()));
  }
  wxIntegerValidator app_id_validator(&application_id_);
  app_id_validator.SetMin(1);
  auto* app_id = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize,
                                lock_id ? wxTE_LEFT | wxTE_READONLY : wxTE_LEFT , app_id_validator);
  app_id->SetMinSize({5*10,-1});

  auto* app_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                     AppNameValidator(&application_name_));
  app_name->SetMinSize({20*10, -1});

  auto* app_desc = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                     wxTextValidator(wxFILTER_NONE, &description_));
  app_desc->SetMinSize({40*10,-1});

  auto* base_name = new wxChoice(this, kIdBaseType, wxDefaultPosition,wxDefaultSize,
                                 MakeBaseList(),0, wxGenericValidator(&base_name_));
  base_name->SetMinSize({20*10,-1});
  base_name->SetSelection(0);

  auto* parent_table = new wxChoice(this, kIdParentTable, wxDefaultPosition,wxDefaultSize,
                                   MakeTableList(model_),0, wxGenericValidator(&parent_));
  parent_table->SetMinSize({20*10,-1});
  parent_table->SetSelection(0);

  auto* db_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                  DatabaseNameValidator(&database_name_));
  db_name->SetMinSize({20*10, -1});

  base_list_ = new wxTreeListCtrl(this, kIdBaseList, wxDefaultPosition,  wxSize( 600, 300 ),
                                  wxTL_CHECKBOX | wxTL_SINGLE);
  base_list_->AppendColumn("Name", 150);
  base_list_->AppendColumn("Database", 100);
  base_list_->AppendColumn("Base", 150);
  base_list_->AppendColumn("Type", 100);
  // base_list_->SetImageList(&image_list_, wxIMAGE_LIST_SMALL);

  auto* save_button_ = new wxButton(this, wxID_OK, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto* cancel_button_ = new wxButton(this, wxID_CANCEL, wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto* app_id_label = new wxStaticText(this, wxID_ANY, L"ID:");
  auto* app_name_label = new wxStaticText(this, wxID_ANY, L"Name:");
  auto* app_desc_label = new wxStaticText(this, wxID_ANY, L"Description:");
  auto* base_name_label = new wxStaticText(this, wxID_ANY, L"Base (ODS) Table Type:");
  auto* parent_table_label = new wxStaticText(this, wxID_ANY, L"Parent Table:");
  auto* db_name_label = new wxStaticText(this, wxID_ANY, L"DB Table Name:");

  int label_width = 100;
  label_width = std::max(label_width,app_id_label->GetBestSize().GetX());
  label_width = std::max(label_width,app_name_label->GetBestSize().GetX());
  label_width = std::max(label_width,app_desc_label->GetBestSize().GetX());
  label_width = std::max(label_width,base_name_label->GetBestSize().GetX());
  label_width = std::max(label_width,parent_table_label->GetBestSize().GetX());
  label_width = std::max(label_width,db_name_label->GetBestSize().GetX());

  app_id_label->SetMinSize({label_width, -1});
  app_name_label->SetMinSize({label_width, -1});
  app_desc_label->SetMinSize({label_width, -1});
  base_name_label->SetMinSize({label_width, -1});
  parent_table_label->SetMinSize({label_width, -1});
  db_name_label->SetMinSize({label_width, -1});

  auto* app_id_sizer = new wxBoxSizer(wxHORIZONTAL);
  app_id_sizer->Add(app_id_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  app_id_sizer->Add(app_id, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* app_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  app_name_sizer->Add(app_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  app_name_sizer->Add(app_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* app_desc_sizer = new wxBoxSizer(wxHORIZONTAL);
  app_desc_sizer->Add(app_desc_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  app_desc_sizer->Add(app_desc, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* base_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  base_name_sizer->Add(base_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  base_name_sizer->Add(base_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* parent_table_sizer = new wxBoxSizer(wxHORIZONTAL);
  parent_table_sizer->Add(parent_table_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  parent_table_sizer->Add(parent_table, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* db_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  db_name_sizer->Add(db_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  db_name_sizer->Add(db_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(save_button_);
  system_sizer->AddButton(cancel_button_);
  system_sizer->Realize();

  auto* app_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Application Configuration");
  app_box->Add(app_id_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  app_box->Add(app_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  app_box->Add(app_desc_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* base_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Table Configuration");
  base_box->Add(base_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  base_box->Add(parent_table_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  base_box->Add(db_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* base_column_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Base Columns");
  base_column_box->Add(base_list_, 1, wxALL | wxEXPAND,  1);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(app_box, 0,  wxALL,  4);
  main_sizer->Add(base_box, 0,  wxALL, 4);
  main_sizer->Add(base_column_box, 1,  wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);
  FetchBaseAttribute();
  SetSizerAndFit(main_sizer);

  auto size = GetBestSize();
  size.IncBy(1);
  SetInitialSize(size);

  save_button_->SetDefault();

}

bool TableDialog::TransferDataToWindow() {
  application_id_ = table_.ApplicationId();
  application_name_ = wxString::FromUTF8(table_.ApplicationName());
  description_ = wxString::FromUTF8(table_.Description());
  base_name_ = BaseIdToUserText(table_.BaseId());
  const auto* parent_table = model_.GetTable(table_.ParentId());
  parent_ = parent_table != nullptr ? parent_table->ApplicationName() : std::string();
  database_name_ = table_.DatabaseName();

  const auto transfer = wxWindowBase::TransferDataToWindow();
  RedrawBaseList();

  return transfer;
}

bool TableDialog::TransferDataFromWindow() {
  const auto valid = wxWindowBase::TransferDataFromWindow();
  if (!valid) {
    return false;
  }
  application_name_.Trim(false).Trim(true);
  description_.Trim(false).Trim(true);
  base_name_.Trim(false).Trim(true);
  parent_.Trim(false).Trim(true);
  database_name_.Trim(false).Trim(true);

    // Check the Application ID > 0
  if (application_id_ <= 0) {
    wxMessageBox(L"Application ID must be larger than 0.",
                 L"Validation Error", wxOK | wxICON_ERROR, this);
    return false;
  }
  // Check that parent column exist and that it has the right reference
  const auto parent_list = GetParentBaseName(table_.BaseId());

  const auto find1 = std::ranges::any_of(attr_list_, [&] (auto& attr) {
    if (!attr.Selected())  {
      return false;
    }
    return std::ranges::any_of(parent_list, [&] (const auto& base) {
      if (util::string::IEquals(base,attr.BaseName())) {
        if (table_.ParentId() > 0) {
          attr.ReferenceId(table_.ParentId());
        }
        return true;
      }
      return false;
    });
  });

  const auto find2 = std::ranges::any_of(table_.Columns(), [&] (const auto& column) {
    return std::ranges::any_of(parent_list, [&] (const auto& base) {
      return util::string::IEquals(base,column.BaseName());
    });
  });

  if (table_.ParentId() <= 0 && !parent_list.empty()) {
    if (table_.BaseId() != BaseId::AoAny && table_.BaseId() != BaseId::AoLog) {
      wxMessageBox(L"This type of table must have a parent table.",
                   L"Validation Error", wxOK | wxICON_ERROR, this);
      return false;
    }
  }
  if (table_.ParentId() > 0 && parent_list.empty()) {
    wxMessageBox(L"This type of table cannot be a child table.",
                 L"Validation Error", wxOK | wxICON_ERROR, this);
    return false;
  }
  if (table_.ParentId() > 0 && !parent_list.empty()) {
    if (!find1 && !find2) {
      wxMessageBox(L"The table is missing a parent column.\nSelect a parent (base) column.",
                   L"Validation Error", wxOK | wxICON_ERROR, this);
      return false;
    }
  }
  table_.ApplicationId(application_id_);

  table_.ApplicationName(application_name_.utf8_string());
  table_.Description(description_.utf8_string());
  table_.DatabaseName(database_name_.utf8_string());
  // BaseId and ParentId is updated by callback functions
  for (const auto& attr : attr_list_) {
    if (attr.Selected()) {
      table_.AddColumn(attr);
    }
  }
  return true;
}
void TableDialog::FetchBaseAttribute() {
  // Fetch all available attributes
  attr_list_ = GetBaseAttributeList(table_.BaseId());
  // Remove all already added to the table
  for (auto itr = attr_list_.begin(); itr != attr_list_.end(); /* No ++itr here */) {
    const auto& attr = *itr;
    const auto* exist = table_.GetColumnByBaseName(attr.BaseName());
    if (exist != nullptr) {
      itr = attr_list_.erase(itr);
    } else {
      ++itr;
    }
  }
  // Set Mandatory to selected by default
  for (auto& attr : attr_list_) {
    if (attr.Mandatory()) {
      attr.Selected(true);
    }
  }
}

void TableDialog::RedrawBaseList() {
  if (base_list_ == nullptr) {
    return;
  }
  long row = 0;
  base_list_->DeleteAllItems();
  auto root = base_list_->GetRootItem();
  for (const auto& attr : attr_list_) {
    auto item = base_list_->AppendItem(root, attr.ApplicationName());
    base_list_->SetItemText(item, 1, attr.DatabaseName());
    base_list_->SetItemText(item, 2,attr.BaseName());
    base_list_->SetItemText(item, 3, DataTypeToUserText(attr.DataType()));
    base_list_->CheckItem(item, attr.Selected() ? wxCHK_CHECKED : wxCHK_UNCHECKED);
  }
}

void TableDialog::OnToggleSelect(wxTreeListEvent& event) {
  if (base_list_ == nullptr) {
    return;
  }
  const auto item = event.GetItem();
  const auto selected = base_list_->GetCheckedState(item) == wxCHK_CHECKED;
  const auto app_name = base_list_->GetItemText(item).ToStdString();
  auto itr = std::ranges::find_if(attr_list_, [&] (const auto& attr) {
    return app_name == attr.ApplicationName();
  });

  if (itr != attr_list_.end()) {
    itr->Selected(selected);
  }
}

void TableDialog::OnBaseChange(wxCommandEvent &event) {
  const auto base_id = TextToBaseId((event.GetString().ToStdString()));
  if (base_id != table_.BaseId() && base_id != BaseId::AoNotDefined) {
    table_.BaseId(base_id);
    FetchBaseAttribute();
    RedrawBaseList();
  }
}

void TableDialog::OnParentChange(wxCommandEvent &event) {
  const auto name = event.GetString().ToStdString();
  const auto* parent = name.empty() ? nullptr : model_.GetTableByName(name);
  table_.ParentId(parent == nullptr ? 0 : parent->ApplicationId());
}


}

