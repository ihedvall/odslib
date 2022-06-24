/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <wx/valgen.h>
#include <wx/combobox.h>
#include <wx/valnum.h>
#include "columndialog.h"
#include "databasenamevalidator.h"
#include "appnamevalidator.h"
#include "odsconfigid.h"
#include "enumdialog.h"
namespace {

wxArrayString MakeTableList(const ods::IModel& model, int64_t exclude) {
  const auto& table_list = model.AllTables();
  wxArrayString list;
  list.Add("");
  for (const auto* table : table_list) {
    if (table == nullptr || table->ApplicationName().empty() || table->ApplicationId() == exclude) {
      continue;
    }
    list.Add(table->ApplicationName());
  }
  list.Sort();
  return list;
}

wxArrayString MakeDataTypeList() {
  wxArrayString list;
  for (int type = 1; type < 50; ++type) {
    const auto dt_type = static_cast<ods::DataType>(type);
    if (dt_type == ods::DataType::DtUnknown) {
      continue;
    }
    const auto dt_text = ods::DataTypeToText(dt_type);
    if (!util::string::IEquals(dt_text, "Dt", 2)) {
      continue;
    }
    list.Add(ods::DataTypeToUserText(dt_type));
  }
  list.Sort();
  return list;
}

wxArrayString MakeEnumList(const ods::IModel& model) {
  const auto& enum_list = model.Enums();
  wxArrayString list;
  list.Add("");
  for (const auto& itr : enum_list) {
    list.Add(itr.second.EnumName());
  }
  list.Sort();
  return list;
}

} // end namespace
namespace ods::gui {

wxBEGIN_EVENT_TABLE(ColumnDialog, wxDialog) //NOLINT
wxEND_EVENT_TABLE()

ColumnDialog::ColumnDialog(wxWindow *parent, IModel& model, const IColumn& original)
    : wxDialog(parent, wxID_ANY, original.ApplicationName().empty() ? L"New Column Dialog" :  L"Edit Column Dialog" ,
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
      model_(model),
      column_(original) {

  auto* app_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                  AppNameValidator(&application_name_));
  app_name->SetMinSize({20*10, -1});

  auto* db_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                  DatabaseNameValidator(&database_name_));
  db_name->SetMinSize({20*10, -1});

  auto* display_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                      wxTextValidator(wxFILTER_NONE, &display_name_));
  display_name->SetMinSize({20*10, -1});

  auto* base_name = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                         wxTextValidator(wxFILTER_NONE, &base_name_));
  base_name->SetMinSize({20*10, -1});

  auto* desc = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                         wxTextValidator(wxFILTER_NONE, &description_));
  desc->SetMinSize({40*10, -1});

  auto* ref_table = new wxChoice(this, kIdRefTable, wxDefaultPosition,wxDefaultSize,
                                 MakeTableList(model_,column_.TableId()),0, wxGenericValidator(&reference_table_));
  ref_table->SetMinSize({20*10,-1});

  auto* ref_col = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                          wxTextValidator(wxFILTER_NONE, &reference_column_));
  ref_col->SetMinSize({20*10, -1});

  auto* data_type = new wxChoice(this, kIdDataType, wxDefaultPosition,wxDefaultSize,
                                 MakeDataTypeList(),0, wxGenericValidator(&data_type_));
  data_type->SetMinSize({20*10,-1});

  auto* enum_name = new wxComboBox(this, kIdEnumName, wxEmptyString, wxDefaultPosition,wxDefaultSize,
                                   MakeEnumList(model_),0, wxGenericValidator(&enum_name_));
  enum_name->SetMinSize({20*10,-1});

  wxIntegerValidator length_validator(&length_);
  length_validator.SetMin(0);
  auto* length = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize,
                                wxTE_LEFT , length_validator);
  length->SetMinSize({5*10,-1});

  wxIntegerValidator nof_decimals_validator(&nof_decimals_);
  nof_decimals_validator.SetMin(-1);
  nof_decimals_validator.SetMax(20);

  auto* nof_dec = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize,
                                wxTE_LEFT , nof_decimals_validator);
  nof_dec->SetMinSize({5*10,-1});

  auto* unit = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                     wxTextValidator(wxFILTER_NONE, &unit_));
  unit->SetMinSize({10*10, -1});

  auto* def_val = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                            wxTextValidator(wxFILTER_NONE, &default_value_));
  def_val->SetMinSize({10*10, -1});

  auto* auto_gen = new wxCheckBox(this, wxID_ANY, "Auto. The database will automatic generate the value.",
                                  wxDefaultPosition,wxDefaultSize, 0, wxGenericValidator(&auto_));
  auto* unique = new wxCheckBox(this, wxID_ANY, "Unique. The column must have a unique value.",
                                wxDefaultPosition,wxDefaultSize, 0, wxGenericValidator(&unique_));
  auto* obligatory = new wxCheckBox(this, wxID_ANY, "Obligatory. The column must have a value.",
                                    wxDefaultPosition,wxDefaultSize, 0, wxGenericValidator(&obligatory_));
  auto* case_sensitive  = new wxCheckBox(this, wxID_ANY, "Case. The column (string) is case-sensitive.",
                                         wxDefaultPosition,wxDefaultSize, 0, wxGenericValidator(&case_sensitive_));
  auto* index = new wxCheckBox(this, wxID_ANY, "Index. The column is indexed.",
                               wxDefaultPosition,wxDefaultSize, 0, wxGenericValidator(&index_));

  auto* save_button_ = new wxButton(this, wxID_OK, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto* cancel_button_ = new wxButton(this, wxID_CANCEL, wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto* app_name_label = new wxStaticText(this, wxID_ANY, L"Application Name:");
  auto* db_name_label = new wxStaticText(this, wxID_ANY, L"Database Name:");
  auto* display_name_label = new wxStaticText(this, wxID_ANY, L"Display Name:");
  auto* base_name_label = new wxStaticText(this, wxID_ANY, L"Base (ODS) Name:");
  auto* desc_label = new wxStaticText(this, wxID_ANY, L"Description:");
  auto* ref_table_label = new wxStaticText(this, wxID_ANY, L"Table:");
  auto* ref_col_label = new wxStaticText(this, wxID_ANY, L"Column:");
  auto* data_type_label = new wxStaticText(this, wxID_ANY, L"Data Type:");
  auto* enum_label = new wxStaticText(this, wxID_ANY, L"Enumerate Name:");
  auto* length_label = new wxStaticText(this, wxID_ANY, L"Data Length:");
  auto* nof_dec_label = new wxStaticText(this, wxID_ANY, L"Number of Decimals:");
  auto* unit_label = new wxStaticText(this, wxID_ANY, L"Unit:");
  auto* default_label = new wxStaticText(this, wxID_ANY, L"Default Value:");

  int label_width = 100;
  label_width = std::max(label_width,app_name_label->GetBestSize().GetX());
  label_width = std::max(label_width,db_name_label->GetBestSize().GetX());
  label_width = std::max(label_width,display_name_label->GetBestSize().GetX());
  label_width = std::max(label_width,base_name_label->GetBestSize().GetX());
  label_width = std::max(label_width,desc_label->GetBestSize().GetX());
  label_width = std::max(label_width,ref_table_label->GetBestSize().GetX());
  label_width = std::max(label_width,ref_col_label->GetBestSize().GetX());
  label_width = std::max(label_width,data_type_label->GetBestSize().GetX());
  label_width = std::max(label_width,enum_label->GetBestSize().GetX());
  label_width = std::max(label_width,length_label->GetBestSize().GetX());
  label_width = std::max(label_width,nof_dec_label->GetBestSize().GetX());
  label_width = std::max(label_width,unit_label->GetBestSize().GetX());
  label_width = std::max(label_width,default_label->GetBestSize().GetX());

  app_name_label->SetMinSize({label_width, -1});
  db_name_label->SetMinSize({label_width, -1});
  display_name_label->SetMinSize({label_width, -1});
  base_name_label->SetMinSize({label_width, -1});
  desc_label->SetMinSize({label_width, -1});
  ref_table_label->SetMinSize({label_width, -1});
  ref_col_label->SetMinSize({label_width, -1});
  data_type_label->SetMinSize({label_width, -1});
  enum_label->SetMinSize({label_width, -1});
  length_label->SetMinSize({label_width, -1});
  nof_dec_label->SetMinSize({label_width, -1});
  unit_label->SetMinSize({label_width, -1});
  default_label->SetMinSize({label_width, -1});

  auto* app_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  app_name_sizer->Add(app_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  app_name_sizer->Add(app_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* db_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  db_name_sizer->Add(db_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  db_name_sizer->Add(db_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* display_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  display_name_sizer->Add(display_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  display_name_sizer->Add(display_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* base_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  base_name_sizer->Add(base_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  base_name_sizer->Add(base_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* desc_sizer = new wxBoxSizer(wxHORIZONTAL);
  desc_sizer->Add(desc_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  desc_sizer->Add(desc, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* ref_table_sizer = new wxBoxSizer(wxHORIZONTAL);
  ref_table_sizer->Add(ref_table_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  ref_table_sizer->Add(ref_table, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* ref_col_sizer = new wxBoxSizer(wxHORIZONTAL);
  ref_col_sizer->Add(ref_col_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  ref_col_sizer->Add(ref_col, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* data_type_sizer = new wxBoxSizer(wxHORIZONTAL);
  data_type_sizer->Add(data_type_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  data_type_sizer->Add(data_type, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* enum_sizer = new wxBoxSizer(wxHORIZONTAL);
  enum_sizer->Add(enum_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  enum_sizer->Add(enum_name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* length_sizer = new wxBoxSizer(wxHORIZONTAL);
  length_sizer->Add(length_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  length_sizer->Add(length, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* nof_dec_sizer = new wxBoxSizer(wxHORIZONTAL);
  nof_dec_sizer->Add(nof_dec_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  nof_dec_sizer->Add(nof_dec, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* unit_sizer = new wxBoxSizer(wxHORIZONTAL);
  unit_sizer->Add(unit_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  unit_sizer->Add(unit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* default_sizer = new wxBoxSizer(wxHORIZONTAL);
  default_sizer->Add(default_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  default_sizer->Add(def_val, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(save_button_);
  system_sizer->AddButton(cancel_button_);
  system_sizer->Realize();

  auto* id_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Identification");
  id_box->Add(app_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  id_box->Add(db_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  id_box->Add(display_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  id_box->Add(base_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  id_box->Add(desc_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* type_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Configuration");
  type_box->Add(data_type_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  type_box->Add(enum_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  type_box->Add(length_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  type_box->Add(nof_dec_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  type_box->Add(unit_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  type_box->Add(default_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* flag_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Flags");
  flag_box->Add(auto_gen, 0, wxALIGN_LEFT | wxALL,  1);
  flag_box->Add(unique, 0, wxALIGN_LEFT | wxALL,  1);
  flag_box->Add(obligatory, 0, wxALIGN_LEFT | wxALL,  1);
  flag_box->Add(case_sensitive, 0, wxALIGN_LEFT | wxALL,  1);
  flag_box->Add(index, 0, wxALIGN_LEFT | wxALL,  1);

  auto* ref_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Reference");
  ref_box->Add(ref_table_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  ref_box->Add(ref_col_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(id_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(type_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(flag_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(ref_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  save_button_->SetDefault();

}

bool ColumnDialog::TransferDataToWindow() {
  application_name_ = wxString::FromUTF8(column_.ApplicationName());
  database_name_ = wxString::FromUTF8(column_.DatabaseName());
  display_name_ = wxString::FromUTF8(column_.DisplayName());
  base_name_ = wxString::FromUTF8(column_.BaseName());
  description_ = wxString::FromUTF8(column_.Description());

  const auto* ref = column_.ReferenceId() > 0 ? model_.GetTable(column_.ReferenceId()) : nullptr;
  reference_table_ = ref != nullptr ? wxString::FromUTF8(ref->ApplicationName()) : wxString();
  reference_column_ = wxString::FromUTF8(column_.ReferenceName());
  data_type_ = DataTypeToUserText(column_.DataType());
  enum_name_ = wxString::FromUTF8(column_.EnumName());
  length_ = column_.DataLength();
  nof_decimals_ = column_.NofDecimals();
  unit_ = wxString::FromUTF8(column_.Unit());
  default_value_ = wxString::FromUTF8(column_.DefaultValue());

  auto_ = column_.Auto();
  unique_ = column_.Unique();
  obligatory_ = column_.Obligatory();
  case_sensitive_ = column_.CaseSensitive();
  index_ = column_.Index();
  return wxWindowBase::TransferDataToWindow();
}

bool ColumnDialog::TransferDataFromWindow() {
  const auto ret = wxWindowBase::TransferDataFromWindow();
  if (!ret) {
    return false;
  }
  application_name_.Trim(true).Trim(false);
  database_name_.Trim(true).Trim(false);
  display_name_.Trim(true).Trim(false);
  base_name_.Trim(true).Trim(false).MakeLower();
  description_.Trim(true).Trim(false);

  reference_column_.Trim(true).Trim(false);
  enum_name_.Trim(true).Trim(false);
  unit_.Trim(true).Trim(false);
  default_value_.Trim(true).Trim(false);

  const auto valid = ValidateColumn();
  if (!valid) {
    return false;
  }

  column_.ApplicationName(application_name_.utf8_string());
  column_.DatabaseName(database_name_.utf8_string());
  column_.DisplayName(display_name_.utf8_string());
  column_.BaseName(base_name_.utf8_string());
  column_.Description(description_.utf8_string());
  if (!reference_table_.IsEmpty()) {
    const auto* ref = model_.GetTableByName(reference_table_.utf8_string());
    column_.ReferenceId(ref != nullptr ? ref->ApplicationId() : 0);
  } else {
    column_.ReferenceId(0);
  }
  column_.ReferenceName(reference_column_.utf8_string());
  column_.DataType(TextToDataType(data_type_.ToStdString()));
  column_.EnumName(enum_name_.utf8_string());
  column_.DataLength(length_);
  column_.NofDecimals(nof_decimals_);
  column_.Unit(unit_.utf8_string());
  column_.DefaultValue(default_value_.utf8_string());
  column_.Auto(auto_);
  column_.Unique(unique_);
  column_.Obligatory(obligatory_);
  column_.CaseSensitive(case_sensitive_);
  column_.Index(index_);
  return true;
}

bool ColumnDialog::ValidateColumn() {
  // Application name may not be empty string
  if (application_name_.IsEmpty()) {
    wxMessageBox(L"The application name cannot be empty!", L"Validation Error",
                 wxOK | wxCENTRE | wxICON_ERROR, this);
    return false;
  }

  // Verify that the enum_name exist. If not ask to create.
  if (!enum_name_.IsEmpty()) {
    const auto* obj = model_.GetEnum(enum_name_.utf8_string());
    if (obj == nullptr) {
      std::ostringstream ask;
      ask << "The enumerate (" << enum_name_ << ") doesn't exist." << std::endl;
      ask << "Do you want to create it?";
      const auto ret = wxMessageBox(ask.str(), L"Validation Error",
                                    wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION, this);
      switch (ret) {
        case wxYES: {
          IEnum new_enum;
          new_enum.EnumName(enum_name_.utf8_string());
          EnumDialog dialog(this, model_, new_enum);
          dialog.SetNameReadOnly();
          const auto create = dialog.ShowModal();
          if (create != wxID_OK) {
            return false;
          }
          model_.AddEnum(new_enum);
          break;
        }

        case wxNO:
          break;

        default:
          return false;
      }
    }
  }

  // Check that data type DtEnum have an enum
  if (TextToDataType(data_type_.ToStdString()) == DataType::DtEnum && enum_name_.IsEmpty()) {
    wxMessageBox(L"The column is an enumerate type so an enumerate name must be defined!", L"Validation Error",
                 wxOK | wxCENTRE | wxICON_ERROR, this);
    return false;
  }

  if (util::string::IEquals(base_name_.ToStdString(),"id")) {
    auto_ = true;
    unique_ = true;
    index_ = true;
   }

  return true;
}

} // end namespace