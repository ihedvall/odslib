/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "relationdialog.h"

#include <set>
#include <wx/valgen.h>

#include <util/stringutil.h>

#include "appnamevalidator.h"

using namespace util::string;

namespace ods::gui {

wxBEGIN_EVENT_TABLE(RelationDialog, wxDialog) // NOLINT
wxEND_EVENT_TABLE()

RelationDialog::RelationDialog(wxWindow *parent, const IModel &model, const IRelation &relation)
    : wxDialog(parent, wxID_ANY, relation.Name().empty() ? L"New Many-to-Many Relation" :  L"Edit Many-to-Many Relation" ,
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      model_(model),
      relation_(relation)
      {
  name_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                   AppNameValidator( &name_));
  name_ctrl_->SetMinSize({30*10, -1});

  auto* table1_ctrl = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              MakeTableList(),0, wxGenericValidator(&table1_) );
  auto* table2_ctrl = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              MakeTableList(),0, wxGenericValidator(&table2_) );

  database_name_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                            AppNameValidator(  &database_name_));
  database_name_ctrl_->SetMinSize({30*10, -1});

  auto* inverse_name_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                           AppNameValidator( &inverse_name_));
  inverse_name_ctrl->SetMinSize({30*10, -1});

  auto* base_name_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                        AppNameValidator( &base_name_));
  base_name_ctrl->SetMinSize({30*10, -1});

  auto* inverse_base_name_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                                AppNameValidator(  &inverse_base_name_));
  inverse_base_name_ctrl->SetMinSize({30*10, -1});

  auto* save_button_ = new wxButton(this, wxID_OK, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto* cancel_button_ = new wxButton(this, wxID_CANCEL, wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto* name_label = new wxStaticText(this, wxID_ANY, L"Reference Name:");
  auto* table1_label = new wxStaticText(this, wxID_ANY, L"Table 1:");
  auto* table2_label = new wxStaticText(this, wxID_ANY, L"Table 2:");
  auto* database_name_label = new wxStaticText(this, wxID_ANY, L"Database Table Name:");
  auto* inverse_name_label = new wxStaticText(this, wxID_ANY, L"Inverse Name:");
  auto* base_name_label = new wxStaticText(this, wxID_ANY, L"Base Name:");
  auto* inverse_base_name_label = new wxStaticText(this, wxID_ANY, L"Inverse Base Name:");

  int label_width = 100;
  label_width = std::max(label_width, name_label->GetBestSize().GetX());
  label_width = std::max(label_width, table1_label->GetBestSize().GetX());
  label_width = std::max(label_width, table2_label->GetBestSize().GetX());
  label_width = std::max(label_width, database_name_label->GetBestSize().GetX());
  label_width = std::max(label_width, inverse_name_label->GetBestSize().GetX());
  label_width = std::max(label_width, base_name_label->GetBestSize().GetX());
  label_width = std::max(label_width, inverse_base_name_label->GetBestSize().GetX());

  name_label->SetMinSize({label_width, -1});
  table1_label->SetMinSize({label_width, -1});
  table2_label->SetMinSize({label_width, -1});
  database_name_label->SetMinSize({label_width, -1});
  inverse_name_label->SetMinSize({label_width, -1});
  base_name_label->SetMinSize({label_width, -1});
  inverse_base_name_label->SetMinSize({label_width, -1});

  auto* name_sizer = new wxBoxSizer(wxHORIZONTAL);
  name_sizer->Add(name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  name_sizer->Add(name_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* table1_sizer = new wxBoxSizer(wxHORIZONTAL);
  table1_sizer->Add(table1_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  table1_sizer->Add(table1_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* table2_sizer = new wxBoxSizer(wxHORIZONTAL);
  table2_sizer->Add(table2_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  table2_sizer->Add(table2_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* database_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  database_name_sizer->Add(database_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  database_name_sizer->Add(database_name_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* inverse_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  inverse_name_sizer->Add(inverse_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  inverse_name_sizer->Add(inverse_name_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* base_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  base_name_sizer->Add(base_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  base_name_sizer->Add(base_name_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* inverse_base_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  inverse_base_name_sizer->Add(inverse_base_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  inverse_base_name_sizer->Add(inverse_base_name_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(save_button_);
  system_sizer->AddButton(cancel_button_);
  system_sizer->Realize();

  auto* cfg_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Main Configuration");
  cfg_box->Add(name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  cfg_box->Add(table1_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  cfg_box->Add(table2_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  cfg_box->Add(database_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* reference_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Support Reference");
  reference_box->Add(inverse_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  reference_box->Add(base_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  reference_box->Add(inverse_base_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(cfg_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(reference_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  save_button_->SetDefault();
  name_ctrl_->SetFocus();
}

bool RelationDialog::TransferDataToWindow() {
  name_ = wxString::FromUTF8( relation_.Name() );

  const auto* table1 = model_.GetTable( relation_.ApplicationId1() );
  if (table1 != nullptr) {
    table1_ = wxString::FromUTF8( table1->ApplicationName() );
  }

  const auto* table2 = model_.GetTable( relation_.ApplicationId2() );
  if (table2 != nullptr) {
    table2_ = wxString::FromUTF8( table2->ApplicationName() );
  }
  database_name_ = wxString::FromUTF8(relation_.DatabaseName());
  inverse_name_ = wxString::FromUTF8(relation_.InverseName());
  base_name_ = wxString::FromUTF8(relation_.BaseName());
  inverse_base_name_ = wxString::FromUTF8(relation_.InverseBaseName());

  return wxWindowBase::TransferDataToWindow();
}

bool RelationDialog::TransferDataFromWindow() {
  const auto ret = wxWindowBase::TransferDataFromWindow();
  if (!ret) {
    return false;
  }
  relation_.Name( Trim(name_.utf8_string()) );

  if (const auto* table1 = model_.GetTableByName( table1_.utf8_string() );
      table1 != nullptr ) {
    relation_.ApplicationId1( table1->ApplicationId() );
  } else {
    relation_.ApplicationId1(0);
  }

  if (const auto* table2 = model_.GetTableByName( table2_.utf8_string() );
      table2 != nullptr ) {
    relation_.ApplicationId2( table2->ApplicationId() );
  } else {
    relation_.ApplicationId2(0);
  }

  relation_.DatabaseName( Trim(database_name_.utf8_string()) );
  relation_.InverseName( Trim(inverse_name_.utf8_string()) );
  relation_.BaseName( Trim(base_name_.utf8_string()) );
  relation_.InverseBaseName( Trim(inverse_base_name_.utf8_string()) );

  return true;
}

wxArrayString RelationDialog::MakeTableList() const {
  wxArrayString list;
  const auto table_list = model_.AllTables();

  for (const auto* table : table_list) {
    if (table == nullptr || table->DatabaseName().empty() || table->ApplicationName().empty()) {
      continue;
    }
    list.Add( wxString::FromUTF8(table->ApplicationName()) );
  }
  list.Sort();
  list.Insert({}, 0);

  return list;
}

void RelationDialog::EndModal(int retCode) {
  if (retCode != wxID_OK) {
    wxDialog::EndModal(retCode);
    return;
  }
  if (const auto& name = relation_.Name();
      name.empty() ) {
    std::ostringstream message;
    message << "The many-to-many relation table name is invalid." << std::endl;
    message << "Error: Reference Name is an empty string." << std::endl;

    wxMessageDialog ask(this, wxString::FromUTF8(message.str()), L"Invalid Name",
                        wxOK | wxOK_DEFAULT | wxICON_ERROR | wxCENTRE);
    ask.ShowModal();
    if (name_ctrl_ != nullptr) {
      name_ctrl_->SetFocus();
    }
    return;
  }

  if ( const auto* exist_db_table = model_.GetTableByDbName( relation_.DatabaseName() );
       exist_db_table != nullptr ) {
    std::ostringstream message;
    message << "The many-to-many relation database table name is invalid." << std::endl;
    message << "Error: Database table already exist. Table: "
            << exist_db_table->ApplicationName() << "/"
            << exist_db_table->DatabaseName() << std::endl;

    wxMessageDialog ask(this, wxString::FromUTF8(message.str()), L"Invalid Name",
                        wxOK | wxOK_DEFAULT | wxICON_ERROR | wxCENTRE);
    ask.ShowModal();
    if (database_name_ctrl_ != nullptr) {
      database_name_ctrl_->SetFocus();
    }
    return;
  }

  std::ostringstream warnings;
  const auto* table1 = model_.GetTable(relation_.ApplicationId1());
  const auto* table2 = model_.GetTable(relation_.ApplicationId2());

  if (table1 == nullptr ) {
    warnings << "Warning: " << "Referenced table 1 doesn't exist" << std::endl;
  }
  if ( table2 == nullptr ) {
    warnings << "Warning: " << "Referenced table 2 doesn't exist" << std::endl;
  }
  if (table1 != nullptr && table2 != nullptr) {
    const auto* column1 = table1->GetColumnByBaseName("id");
    const auto* column2 = table2->GetColumnByBaseName("id");
    if (column1 != nullptr && column2 != nullptr
    && IEquals(column1->DatabaseName(), column2->DatabaseName())) {
      warnings << "Warning: " << "Referenced tables have the same ID column name. ID: "
        << column1->DatabaseName() << std::endl;
    }
  }
  if ( relation_.DatabaseName().empty() ) {
    warnings << "Warning: " << "There is no database table name." << std::endl;
  }
  if (!warnings.str().empty()) {
    std::ostringstream message;
    message << "The many-to-many relation may be invalid. Do you want to save the changes ?" << std::endl;
    message << warnings.str();
    wxMessageDialog ask(this, wxString::FromUTF8(message.str()), L"Invalid Configuration",
                        wxYES_NO | wxYES_DEFAULT | wxICON_WARNING | wxCENTRE);
    ask.SetYesNoLabels(wxMessageDialog::ButtonLabel(wxID_SAVE),wxMessageDialog::ButtonLabel(wxID_NO) );
    const auto ask_code = ask.ShowModal();

    if (ask_code == wxID_NO) {
      return;
    }

  }

  wxDialog::EndModal(retCode);
}

} // ods::gui