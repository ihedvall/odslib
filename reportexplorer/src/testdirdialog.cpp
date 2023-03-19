/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include "testdirdialog.h"
#include "reportexplorer.h"


namespace ods::gui {

wxBEGIN_EVENT_TABLE(TestDirDialog, wxDialog) //NOLINT
wxEND_EVENT_TABLE()

TestDirDialog::TestDirDialog(wxWindow *parent, detail::TestDirectory& test_dir)
    : wxDialog(parent, wxID_ANY, "Test Directory Dialog" ,
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
      test_dir_(test_dir) {

  auto& app = wxGetApp();
  const auto exists = app.GetEnv(test_dir.Name()) != nullptr;
  bool db_exist = false;
  bool model_exist = false;
  try {
    std::filesystem::path db_path(test_dir.DbFileName());
    db_exist = std::filesystem::exists(db_path);

    std::filesystem::path model_path(test_dir.ModelFileName());
    model_exist = std::filesystem::exists(model_path);
  } catch (const std::exception& ) {

  }

  auto *name = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                              exists ? wxTE_READONLY : 0, wxTextValidator(wxFILTER_EMPTY, &name_));
  name->SetMinSize({20 * 10, -1});


  auto *desc = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0,
                              wxTextValidator(wxFILTER_NONE, &description_));
  desc->SetMinSize({80 * 10, -1});

  model_file_ = new wxFilePickerCtrl(this, wxID_ANY, test_dir_.ModelFileName(),
                                          "Select ODS Model File", "ODS Model Files (*.xml)|*.xml|All Files (*.*)|*.*",
                                          wxDefaultPosition, wxDefaultSize,
                                          wxFLP_OPEN | wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);
  model_file_->SetMinSize({80 * 10, -1});
  model_file_->Enable(!model_exist);

  root_dir_ = new wxDirPickerCtrl(this, wxID_ANY, test_dir_.RootDir(), "Select Test Root Directory",
                                       wxDefaultPosition, wxDefaultSize,
                                       wxDIRP_DIR_MUST_EXIST | wxPB_USE_TEXTCTRL);
  root_dir_->SetMinSize({80 * 10, -1});

  db_file_ = new wxFilePickerCtrl(this, wxID_ANY, test_dir_.DbFileName(),
                                       "Select Database File", "SQLite Files (*.sqlite)|*.sqlite|All Files (*.*)|*.*",
                                       wxDefaultPosition, wxDefaultSize,
                                       wxFLP_SAVE | wxFLP_USE_TEXTCTRL );
  db_file_->SetMinSize({80 * 10, -1});
  db_file_->Enable(!db_exist);

  auto *format = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0,
                                wxTextValidator(wxFILTER_NONE, &test_dir_format_));
  format->SetMinSize({40 * 10, -1});

  auto *exclude = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0,
                                 wxTextValidator(wxFILTER_NONE, &exclude_list_));
  exclude->SetMinSize({80 * 10, -1});

  auto *save_button_ = new wxButton(this, wxID_OK, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto *cancel_button_ = new wxButton(this, wxID_CANCEL, wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto *name_label = new wxStaticText(this, wxID_ANY, L"Name:");
  auto *desc_label = new wxStaticText(this, wxID_ANY, L"Description:");
  auto *model_file_label = new wxStaticText(this, wxID_ANY, L"ODS Model File:");
  auto *root_dir_label = new wxStaticText(this, wxID_ANY, L"Root Directory:");
  auto *db_file_label = new wxStaticText(this, wxID_ANY, L"Database File:");
  auto *format_label = new wxStaticText(this, wxID_ANY, L"Test Directory Format:");
  auto *exclude_label = new wxStaticText(this, wxID_ANY, L"Exclude:");

  int label_width = 100;
  label_width = std::max(label_width, name_label->GetBestSize().GetX());
  label_width = std::max(label_width, desc_label->GetBestSize().GetX());
  label_width = std::max(label_width, model_file_label->GetBestSize().GetX());
  label_width = std::max(label_width, root_dir_label->GetBestSize().GetX());
  label_width = std::max(label_width, db_file_label->GetBestSize().GetX());
  label_width = std::max(label_width, format_label->GetBestSize().GetX());
  label_width = std::max(label_width, exclude_label->GetBestSize().GetX());

  name_label->SetMinSize({label_width, -1});
  desc_label->SetMinSize({label_width, -1});
  root_dir_label->SetMinSize({label_width, -1});
  db_file_label->SetMinSize({label_width, -1});
  model_file_label->SetMinSize({label_width, -1});
  format_label->SetMinSize({label_width, -1});
  exclude_label->SetMinSize({label_width, -1});

  auto *name_sizer = new wxBoxSizer(wxHORIZONTAL);
  name_sizer->Add(name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  name_sizer->Add(name, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *desc_sizer = new wxBoxSizer(wxHORIZONTAL);
  desc_sizer->Add(desc_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  desc_sizer->Add(desc, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *model_file_sizer = new wxBoxSizer(wxHORIZONTAL);
  model_file_sizer->Add(model_file_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  model_file_sizer->Add(model_file_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *root_dir_sizer = new wxBoxSizer(wxHORIZONTAL);
  root_dir_sizer->Add(root_dir_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  root_dir_sizer->Add(root_dir_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *db_file_sizer = new wxBoxSizer(wxHORIZONTAL);
  db_file_sizer->Add(db_file_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  db_file_sizer->Add(db_file_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *format_sizer = new wxBoxSizer(wxHORIZONTAL);
  format_sizer->Add(format_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  format_sizer->Add(format, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *exclude_sizer = new wxBoxSizer(wxHORIZONTAL);
  exclude_sizer->Add(exclude_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  exclude_sizer->Add(exclude, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto *system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(save_button_);
  system_sizer->AddButton(cancel_button_);
  system_sizer->Realize();

  auto *id_box = new wxStaticBoxSizer(wxVERTICAL, this, L"Environment Identification");
  id_box->Add(name_sizer, 0, wxALIGN_LEFT | wxALL, 1);
  id_box->Add(desc_sizer, 0, wxALIGN_LEFT | wxALL, 1);
  id_box->Add(model_file_sizer, 0, wxALIGN_LEFT | wxALL, 1);

  auto *cfg_box = new wxStaticBoxSizer(wxVERTICAL, this, L"Test Directory Configuration");
  cfg_box->Add(root_dir_sizer, 0, wxALIGN_LEFT | wxALL, 1);
  cfg_box->Add(db_file_sizer, 0, wxALIGN_LEFT | wxALL, 1);
  cfg_box->Add(format_sizer, 0, wxALIGN_LEFT | wxALL, 1);
  cfg_box->Add(exclude_sizer, 0, wxALIGN_LEFT | wxALL, 1);

  auto *main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(id_box, 0, wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  main_sizer->Add(cfg_box, 0, wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  if (exists) {
    cancel_button_->SetDefault();
  } else {
    save_button_->SetDefault();
  }
}

bool TestDirDialog::TransferDataToWindow() {
  name_ = wxString::FromUTF8(test_dir_.Name());
  description_ = wxString::FromUTF8(test_dir_.Description());
  model_file_->SetPath(wxString::FromUTF8(test_dir_.ModelFileName()));
  root_dir_->SetPath(wxString::FromUTF8(test_dir_.RootDir()));
  db_file_->SetPath(wxString::FromUTF8(test_dir_.DbFileName()));
  test_dir_format_ = wxString::FromUTF8(test_dir_.TestDirFormat());
  exclude_list_ = wxString::FromUTF8(test_dir_.ExcludeListToText());

  return wxWindowBase::TransferDataToWindow();
}

bool TestDirDialog::TransferDataFromWindow() {
  const auto ret = wxWindowBase::TransferDataFromWindow();
  if (!ret) {
    return false;
  }

  name_.Trim(true).Trim(false);
  description_.Trim(true).Trim(false);

  auto model = model_file_->GetPath();
  model.Trim(true).Trim(false);

  auto root = root_dir_->GetPath();
  root.Trim(true).Trim(false);

  auto db_file = db_file_->GetPath();
  db_file.Trim(true).Trim(false);

  test_dir_format_.Trim(true).Trim(false);

  test_dir_.Name(name_.utf8_string());
  test_dir_.Description(description_.utf8_string());
  test_dir_.ModelFileName(model.utf8_string());
  test_dir_.RootDir(root.utf8_string());
  test_dir_.DbFileName(db_file.utf8_string());
  test_dir_.TestDirFormat(test_dir_format_.utf8_string());
  test_dir_.TextToExcludeList(exclude_list_.utf8_string());
  return true;
}

} // End namespace
