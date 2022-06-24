/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <unordered_set>
#include <wx/docmdi.h>
#include <wx/splitter.h>
#include "enumpanel.h"
#include "odsconfigid.h"
#include "enumitemdialog.h"
#include "enumdialog.h"
#include "odsconfig.h"

namespace ods::gui {

wxBEGIN_EVENT_TABLE(EnumPanel, wxPanel) //NOLINT
  EVT_LIST_ITEM_SELECTED(kIdEnumList, EnumPanel::OnEnumSelect)
  EVT_LIST_ITEM_ACTIVATED(kIdEnumList, EnumPanel::OnDoubleClickEnum)
  EVT_LIST_ITEM_ACTIVATED(kIdItemList, EnumPanel::OnDoubleClickItem)
  EVT_CONTEXT_MENU( EnumPanel::OnRightClick)

  EVT_UPDATE_UI(kIdCopyEnum, EnumPanel::OnUpdateEnumSelected)
  EVT_UPDATE_UI(kIdPasteEnum, EnumPanel::OnUpdateCopyEnumExist)
  EVT_MENU(kIdCopyEnum, EnumPanel::OnCopyEnum)
  EVT_MENU(kIdPasteEnum, EnumPanel::OnPasteEnum)
wxEND_EVENT_TABLE()

EnumPanel::EnumPanel(wxWindow *parent)
: wxPanel(parent),
  image_list_(16,16,false,2) {
  image_list_.Add(wxBitmap("ENUM_LIST", wxBITMAP_TYPE_BMP_RESOURCE));
  auto *splitter = new wxSplitterWindow(this);
  left_ = new wxListView(splitter, kIdEnumList, wxDefaultPosition, {500, 600},
                         wxLC_REPORT | wxLC_SINGLE_SEL);
  left_->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
  left_->AppendColumn("Summary", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);
  left_->SetImageList(&image_list_, wxIMAGE_LIST_SMALL);

  right_ = new wxListView(splitter, kIdItemList, wxDefaultPosition, {300, 600}, wxLC_REPORT);
  right_->AppendColumn("Item", wxLIST_FORMAT_LEFT, 50);
  right_->AppendColumn("Text", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);

  splitter->SplitVertically(left_, right_, 500);

  auto* main_sizer = new wxBoxSizer(wxHORIZONTAL);
  main_sizer->Add(splitter, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  SetSizerAndFit(main_sizer);

  RedrawEnumList();
  RedrawItemList();
}

void EnumPanel::RedrawEnumList() {
  auto *doc = GetDocument();
  if (doc == nullptr || left_ == nullptr) {
    return;
  }
  std::string selected_enum;
  auto selected = left_->GetFirstSelected();
  if (selected >= 0) {
    selected_enum = left_->GetItemText(selected, 0).ToStdString();
  }

  left_->DeleteAllItems();

  const auto &model = doc->GetModel();

  long row = 0;
  selected = -1;
  const auto& enum_list = model.Enums();
  for (const auto& itr : enum_list) {
    const IEnum& obj = itr.second;
    if (!selected_enum.empty() && obj.EnumName() == selected_enum) {
      selected = row;
    }
    std::ostringstream summary;
    const auto& item_list = obj.Items();
    for (const auto& item : item_list) {
      if (!summary.str().empty()) {
        summary << ", ";
      }
      summary << item.first <<": " << item.second;
      if (summary.str().size() > 30) {
        break;
      }
    }
    left_->InsertItem(row, obj.EnumName(), obj.Locked() ? 0 : 1);
    left_->SetItem(row, 1, wxString::FromUTF8(summary.str()));
    ++row;
  }
  if (selected >= 0) {
    left_->Select(selected);
    left_->EnsureVisible(selected);
  }
}

void EnumPanel::RedrawItemList() {
  if (right_ == nullptr) {
    return;
  }
  std::unordered_set<int64_t> selected_list;
  for (auto sel = right_->GetFirstSelected(); sel >= 0; sel = right_->GetNextSelected(sel)) {
    int64_t key = 0;
    right_->GetItemText(sel).ToLongLong(&key);
    selected_list.insert(key);
  }
  right_->DeleteAllItems();
  const auto* selected_enum = GetSelectedEnum();
  if (selected_enum == nullptr) {
    return;
  }
  long row = 0;
  const auto& item_list = selected_enum->Items();
  for (const auto& itr : item_list) {
    right_->InsertItem(row, std::to_string(itr.first));
    right_->SetItem(row, 1, wxString::FromUTF8(itr.second));
    if (selected_list.find(itr.first) != selected_list.cend()) {
      right_->Select(row);
    }
    ++row;
  }
}

OdsDocument *EnumPanel::GetDocument() const {
  const auto *child_frame = wxDynamicCast(GetGrandParent(), wxDocMDIChildFrame); // NOLINT
  return child_frame != nullptr ? wxDynamicCast(child_frame->GetDocument(), OdsDocument) : nullptr; //NOLINT
}

void EnumPanel::Update() {
  RedrawEnumList();
  RedrawItemList();
  wxWindow::Update();
}

void EnumPanel::OnEnumSelect(wxListEvent&) {
  RedrawItemList();
}

void EnumPanel::OnRightClick(wxContextMenuEvent& event) {
  switch (event.GetId()) {
    case kIdEnumList: {
      wxMenu menu;
      menu.Append(kIdAddEnum,wxGetStockLabel(wxID_ADD));
      menu.Append(kIdEditEnum, wxGetStockLabel(wxID_EDIT));
      menu.Append(kIdDeleteEnum, wxGetStockLabel(wxID_DELETE));
      menu.AppendSeparator();
      menu.Append(kIdCopyEnum, wxGetStockLabel(wxID_COPY));
      menu.Append(kIdPasteEnum, wxGetStockLabel(wxID_PASTE));
      PopupMenu(&menu);
      break;
    }

    case kIdItemList: {
      wxMenu menu;
      menu.Append(kIdAddEnumItem,wxGetStockLabel(wxID_ADD));
      menu.Append(kIdEditEnumItem, wxGetStockLabel(wxID_EDIT));
      menu.Append(kIdDeleteEnumItem, wxGetStockLabel(wxID_DELETE));
      PopupMenu(&menu);
      break;
    }

    default:
      break;
  }

}

void EnumPanel::OnUpdateSingleEnumSelected(wxUpdateUIEvent &event) {
  if (left_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(left_->GetSelectedItemCount() == 1);
}

void EnumPanel::OnUpdateEnumSelected(wxUpdateUIEvent &event) {
  if (left_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(left_->GetSelectedItemCount() > 0);
}

void EnumPanel::OnAddEnum(wxCommandEvent &event) {
  auto* doc = GetDocument();
  if (doc == nullptr) {
    return;
  }
  auto& model = doc->GetModel();
  IEnum empty;
  EnumDialog dialog(this, model, empty);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  model.AddEnum(dialog.GetEnum());
  RedrawEnumList();
  SelectEnum(dialog.GetEnum().EnumName());
}

void EnumPanel::OnEditEnum(wxCommandEvent &) {
  auto* doc = GetDocument();
  auto* selected_enum = GetSelectedEnum();
  if (selected_enum == nullptr || doc == nullptr) {
    return;
  }
  auto& model = doc->GetModel();
  EnumDialog dialog(this, model, *selected_enum);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  const auto& after = dialog.GetEnum();
  if (selected_enum->EnumName() != after.EnumName()) {
    wxMessageDialog ask(this,
     L"The enumerate name has changed.\nDo you want to create a new or modify the existing one? ",
     L"Create or Modify Enumerate", wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION | wxCENTRE);
    ask.SetYesNoLabels(L"Create New", L"Modify");
    const auto ret1 = ask.ShowModal();
    if (ret1 == wxID_YES) {
      model.AddEnum(after);
    } else if (ret1 == wxID_NO) {
      model.DeleteEnum(selected_enum->EnumName());
      model.AddEnum(after);
    }
  } else {
    *selected_enum = after;
  }
  RedrawEnumList();
}

void EnumPanel::OnDeleteEnum(wxCommandEvent &event) {
  auto* doc = GetDocument();
  if (left_ == nullptr || doc == nullptr) {
    return;
  }

  std::vector<std::string> del_list;
  for (auto item = left_->GetFirstSelected(); item >= 0; item = left_->GetNextSelected(item)) {
    std::string value = left_->GetItemText(item).utf8_string();
    del_list.emplace_back(value);
  }
  if (del_list.empty()) {
    return;
  }

  std::ostringstream ask;
  ask << "Do you want to delete the following enumerates?";
  for (const auto& del : del_list) {
    ask << std::endl << del;
  }

  int ret = wxMessageBox(ask.str(), "Delete Enumerate Dialog",
                         wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION,
                         this);

  if (ret != wxYES) {
    return;
  }
  auto& model = doc->GetModel();
  for (const auto& del : del_list) {
    model.DeleteEnum(del);
  }
  RedrawEnumList();
}

void EnumPanel::OnUpdateSingleEnumItemSelected(wxUpdateUIEvent &event) {
  if (left_ == nullptr || right_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(left_->GetSelectedItemCount() == 1 && right_->GetSelectedItemCount() == 1);
}

void EnumPanel::OnUpdateEnumItemSelected(wxUpdateUIEvent &event) {
  if (left_ == nullptr || right_ == nullptr) {
    event.Enable(false);
    return;
  }
  event.Enable(left_->GetSelectedItemCount() == 1 && right_->GetSelectedItemCount() > 0);
}

void EnumPanel::OnAddEnumItem(wxCommandEvent &event) {
 auto* selected_enum = GetSelectedEnum();
 if (selected_enum == nullptr) {
   return;
 }
 EnumItemDialog dialog(this);
 dialog.key_ = selected_enum->GetNextKey();
 const auto ret = dialog.ShowModal();
 if (ret != wxID_OK) {
   return;
 }
 selected_enum->AddItem(dialog.key_, dialog.value_.utf8_string());
 RedrawItemList();
}

void EnumPanel::OnEditEnumItem(wxCommandEvent &) {
  auto* selected_enum = GetSelectedEnum();
  if (selected_enum == nullptr) {
    return;
  }

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
    selected_enum->DeleteItem(key);
  }
  selected_enum->AddItem(dialog.key_, dialog.value_.utf8_string());
  RedrawItemList();
}

void EnumPanel::OnDeleteEnumItem(wxCommandEvent &event) {
  auto* selected_enum = GetSelectedEnum();
  if (selected_enum == nullptr || right_ == nullptr) {
    return;
  }
  std::map<int64_t,std::string> del_list;
  for (auto item = right_->GetFirstSelected(); item >= 0; item = right_->GetNextSelected(item)) {
    int64_t key = 0;
    right_->GetItemText(item).ToLongLong(&key);
    std::string value = right_->GetItemText(item, 1).utf8_string();
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
    selected_enum->DeleteItem(del.first);
  }
  RedrawItemList();
}

void EnumPanel::OnDoubleClickEnum(wxListEvent &event) {
  wxCommandEvent dummy;
  OnEditEnum(dummy);
}

void EnumPanel::OnDoubleClickItem(wxListEvent &event) {
  wxCommandEvent dummy;
  OnEditEnumItem(dummy);
}

IEnum *EnumPanel::GetSelectedEnum() const {
  auto* doc = GetDocument();
  if (doc == nullptr || left_ == nullptr) {
    return nullptr;
  }
  const auto& model = doc->GetModel();

  const auto item = left_->GetFirstSelected();
  if (item < 0 || left_->GetSelectedItemCount() > 1) {
    return nullptr;
  }
  const auto name = left_->GetItemText(item).ToStdString();
  const auto* enum_obj = model.GetEnum(name);
  if (enum_obj == nullptr) {
    return nullptr;
  }
  return const_cast<IEnum*>(model.GetEnum(name));
}

bool EnumPanel::GetSelectedItem(int64_t& key, std::string& value) const {
  auto* selected_enum = GetSelectedEnum();
  if (selected_enum == nullptr || right_ == nullptr) {
    return false;
  }
  const auto& item_list = selected_enum->Items();
  const auto item = right_->GetFirstSelected();
  if (item < 0 || right_->GetSelectedItemCount() > 1) {
    return false;
  }
  right_->GetItemText(item).ToLongLong(&key);
  value = right_->GetItemText(item, 1).utf8_string();
  return true;
}


void EnumPanel::OnUpdateCopyEnumExist(wxUpdateUIEvent &event) {
  auto& app = wxGetApp();
  event.Enable(!app.CopyEnum().EnumName().empty());
}

void EnumPanel::OnCopyEnum(wxCommandEvent& event) {
  const auto* selected = GetSelectedEnum();
  auto& app = wxGetApp();
  if (selected != nullptr) {
    app.CopyEnum(*selected);
  }
}

void EnumPanel::OnPasteEnum(wxCommandEvent& event) {
  auto* doc = GetDocument();
  if (doc == nullptr) {
    return;
  }
  auto& app = wxGetApp();
  const auto& enumerate = app.CopyEnum();
  if (enumerate.EnumName().empty()) {
    return;
  }
  auto& model = doc->GetModel();

  std::string selected_name = enumerate.EnumName();

  const bool exist = model.GetEnum(selected_name) != nullptr;
  if (exist) {
    std::ostringstream ask;
    ask << "The enumerate already exist in the model. Name: " << selected_name << std::endl;
    ask << "Do you want to replace the existing one?";
    const auto ret = wxMessageBox(ask.str(), "Replace", wxYES_NO | wxNO_DEFAULT | wxCENTRE | wxICON_WARNING, this );
    if (ret != wxYES) {
      return;
    }
  }
  model.AddEnum(enumerate);
  RedrawEnumList();

  const IEnum empty;
  app.CopyEnum(empty); // Reset the copy enumerate
  SelectEnum(selected_name);
}

void EnumPanel::SelectEnum(const std::string& name) {
  if (left_ == nullptr) {
    return;
  }
  std::string selected_name;
  for (long item = left_->GetNextItem(-1) ; item >= 0; item = left_->GetNextItem(item)) {
    const auto enum_name = left_->GetItemText(item).utf8_string();
    const bool match = util::string::IEquals(enum_name, name);
    const bool selected = left_->IsSelected(item);
    if (match && !selected) {
      left_->Select(item);
      selected_name = enum_name;
      left_->EnsureVisible(item);
    } else if (!match && selected) {
      left_->Select(item, false);
    }
  }
  RedrawItemList();
}

} // end namespace