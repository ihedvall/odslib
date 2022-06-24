/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <unordered_set>
#include <wx/valgen.h>
#include "enumdialog.h"
#include "odsconfigid.h"
#include "enumitemdialog.h"
#include "appnamevalidator.h"
namespace ods::gui {

wxBEGIN_EVENT_TABLE(EnumDialog, wxDialog) // NOLINT
  EVT_CONTEXT_MENU(EnumDialog::OnRightClick)
  EVT_LIST_ITEM_ACTIVATED(kIdEnumDialogList, EnumDialog::OnItemDoubleClick)
  EVT_UPDATE_UI(kIdEnumDialogEdit,EnumDialog::OnItemSingleSelect)
  EVT_UPDATE_UI(kIdEnumDialogDelete,EnumDialog::OnItemSelect)
  EVT_MENU(kIdEnumDialogAdd, EnumDialog::OnItemAdd)
  EVT_MENU(kIdEnumDialogEdit, EnumDialog::OnItemEdit)
  EVT_MENU(kIdEnumDialogDelete, EnumDialog::OnItemDelete)
wxEND_EVENT_TABLE()

EnumDialog::EnumDialog(wxWindow *parent, const IModel &model, const IEnum &enumerate)
: wxDialog(parent, wxID_ANY, enumerate.EnumName().empty() ? L"New Enumerate Dialog" :  L"Edit Enumerate Dialog" ,
          wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
          model_(model),
  original_(enumerate),
  enumerate_(enumerate) {
  enum_name_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxDefaultSize, 0,
                                  AppNameValidator(&enum_name_));
  enum_name_ctrl_->SetMinSize({20*10, -1});

  auto* locked = new wxCheckBox(this, wxID_ANY,L"Block On-Line Changes",wxDefaultPosition,wxDefaultSize, 0,
                                   wxGenericValidator(&locked_));
  locked->SetMinSize({40*10, -1});

  item_list_ = new wxListView(this, kIdEnumDialogList, wxDefaultPosition, {400, 400});
  item_list_->AppendColumn(L"Key", wxLIST_FORMAT_LEFT, 50);
  item_list_->AppendColumn(L"Value", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);

  auto* save_button_ = new wxButton(this, wxID_OK, wxGetStockLabel(wxID_SAVE, wxSTOCK_FOR_BUTTON));
  auto* cancel_button_ = new wxButton(this, wxID_CANCEL, wxGetStockLabel(wxID_CANCEL, wxSTOCK_FOR_BUTTON));

  auto* enum_name_label = new wxStaticText(this, wxID_ANY, L"Name:");

  int label_width = 100;
  label_width = std::max(label_width,enum_name_label->GetBestSize().GetX());

  enum_name_label->SetMinSize({label_width, -1});

  auto* enum_name_sizer = new wxBoxSizer(wxHORIZONTAL);
  enum_name_sizer->Add(enum_name_label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  enum_name_sizer->Add(enum_name_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

  auto* system_sizer = new wxStdDialogButtonSizer();
  system_sizer->AddButton(save_button_);
  system_sizer->AddButton(cancel_button_);
  system_sizer->Realize();

  auto* cfg_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Configuration");
  cfg_box->Add(enum_name_sizer, 0, wxALIGN_LEFT | wxALL,  1);
  cfg_box->Add(locked, 0, wxALIGN_LEFT | wxALL,  1);

  auto* item_box = new wxStaticBoxSizer(wxVERTICAL,this, L"Items");
  item_box->Add(item_list_, 1, wxALIGN_LEFT | wxALL | wxEXPAND,  1);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(cfg_box, 0,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(item_box, 1,  wxALIGN_LEFT| wxALL | wxEXPAND, 4);
  main_sizer->Add(system_sizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxLEFT | wxRIGHT, 10);

  SetSizerAndFit(main_sizer);
  save_button_->SetDefault();
}

bool EnumDialog::TransferDataToWindow() {
  enum_name_ = enumerate_.EnumName();
  locked_ = enumerate_.Locked();
  RedrawItemList();
  return wxWindowBase::TransferDataToWindow();
}

bool EnumDialog::TransferDataFromWindow() {
  const auto ret = wxWindowBase::TransferDataFromWindow();
  if (!ret) {
    return false;
  }
  enumerate_.EnumName(enum_name_.utf8_string());
  enumerate_.Locked(locked_);
  // Items list updated direct on enumerate object.
  return true;
}

void EnumDialog::RedrawItemList() {
  if (item_list_ == nullptr) {
    return;
  }
  std::unordered_set<int64_t> selected;
  for (auto item = item_list_->GetFirstSelected(); item>= 0; item = item_list_->GetNextSelected(item)) {
    int64_t key = 0;
    item_list_->GetItemText(item).ToLongLong(&key);
    selected.insert(key);
  }
  item_list_->DeleteAllItems();

  const auto& list = enumerate_.Items();
  long row = 0;
  for (const auto& itr : list) {
    item_list_->InsertItem(row, std::to_string(itr.first));
    item_list_->SetItem(row, 1, wxString::FromUTF8(itr.second));
    if (selected.find(itr.first) != selected.cend()) {
      item_list_->Select(row);
    }
    ++row;
  }
}

void EnumDialog::OnRightClick(wxContextMenuEvent& event) {
  if (event.GetId() == kIdEnumDialogList) {
    wxMenu menu;
    menu.Append(kIdEnumDialogAdd, L"Add Item");
    menu.Append(kIdEnumDialogEdit, L"Edit Item");
    menu.Append(kIdEnumDialogDelete, L"Delete Item");
    PopupMenu(&menu);
  }
}

void EnumDialog::OnItemSingleSelect(wxUpdateUIEvent &event) {
  if (item_list_ == nullptr) {
    event.Enable(false);
  } else {
    event.Enable(item_list_->GetSelectedItemCount() == 1);
  }
}

void EnumDialog::OnItemSelect(wxUpdateUIEvent &event) {
  if (item_list_ == nullptr) {
    event.Enable(false);
  } else {
    event.Enable(item_list_->GetSelectedItemCount() >  0);
  }
}
void EnumDialog::OnItemAdd(wxCommandEvent &event) {

  EnumItemDialog dialog(this);
  dialog.key_ = enumerate_.GetNextKey();
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  enumerate_.AddItem(dialog.key_, dialog.value_.utf8_string());
  RedrawItemList();
}

void EnumDialog::OnItemEdit(wxCommandEvent &) {
  int64_t key = 0;
  std::string value;
  const auto find = GetSelectedItem(key, value);
  if (!find) {
    return;
  }

  EnumItemDialog dialog(this);
  dialog.key_ = key;
  dialog.value_ = value;

  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  if (key != dialog.key_) {
    enumerate_.DeleteItem(key);
  }
  enumerate_.AddItem(dialog.key_, dialog.value_.utf8_string());
  RedrawItemList();
}

void EnumDialog::OnItemDelete(wxCommandEvent &event) {
  if (item_list_ == nullptr) {
    return;
  }
  std::map<int64_t,std::string> del_list;
  for (auto item = item_list_->GetFirstSelected(); item >= 0; item = item_list_->GetNextSelected(item)) {
    int64_t key = 0;
    item_list_->GetItemText(item).ToLongLong(&key);
    std::string value = item_list_->GetItemText(item, 1).utf8_string();
    del_list.insert({key, value});
  }
  if (del_list.empty()) {
    return;
  }

  std::ostringstream ask;
  ask << "Do you want to delete the following enumerate items?";
  for (const auto& del : del_list) {
    ask << std::endl << del.first << ": " << del.second;
  }

  int ret = wxMessageBox(ask.str(), "Delete Enumerate Items Dialog",
                         wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION,
                         this);

  if (ret != wxYES) {
    return;
  }
  for (const auto& del : del_list) {
    enumerate_.DeleteItem(del.first);
  }
  RedrawItemList();
}

void EnumDialog::OnItemDoubleClick(wxListEvent &event) {
  wxCommandEvent dummy;
  OnItemEdit(dummy);
}

bool EnumDialog::GetSelectedItem(int64_t& key, std::string& value) const {
  if (item_list_ == nullptr) {
    return false;
  }
  const auto& item_list = enumerate_.Items();
  const auto item = item_list_->GetFirstSelected();
  if (item < 0 || item_list_->GetSelectedItemCount() > 1) {
    return false;
  }
  item_list_->GetItemText(item).ToLongLong(&key);
  value = item_list_->GetItemText(item, 1).utf8_string();
  return true;
}
} // end namespace
