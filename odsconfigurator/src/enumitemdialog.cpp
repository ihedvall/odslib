/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <wx/wx.h>
#include <wx/valnum.h>
#include "enumitemdialog.h"

namespace ods::gui {
EnumItemDialog::EnumItemDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, L"Enumerate Item Dialog") {

  auto* key = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                  wxIntegerValidator(&key_));
  key->SetMinSize({5*10, -1});

  auto* value = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                  wxTextValidator(wxFILTER_NONE, &value_));
  value->SetMinSize({20*10, -1});

  auto* save_button_ = new wxButton(this, wxID_OK, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto* cancel_button_ = new wxButton(this, wxID_CANCEL, wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto* key_label = new wxStaticText(this, wxID_ANY, L"Key:");
  auto* value_label = new wxStaticText(this, wxID_ANY, L"Value:");

  int label_width = 100;
  label_width = std::max(label_width,key_label->GetBestSize().GetX());
  label_width = std::max(label_width,value_label->GetBestSize().GetX());

  key_label->SetMinSize({label_width, -1});
  value_label->SetMinSize({label_width, -1});

  auto* key_sizer = new wxBoxSizer(wxHORIZONTAL);
  key_sizer->Add(key_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  key_sizer->Add(key, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* value_sizer = new wxBoxSizer(wxHORIZONTAL);
  value_sizer->Add(value_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  value_sizer->Add(value, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(save_button_);
  system_sizer->AddButton(cancel_button_);
  system_sizer->Realize();

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(key_sizer, 0,  wxALIGN_LEFT| wxALL, 4);
  main_sizer->Add(value_sizer, 0,  wxALIGN_LEFT| wxALL, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  save_button_->SetDefault();
  value->SetFocus();
}

} // end namespace