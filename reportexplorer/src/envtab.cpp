/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */


#include <wx/valgen.h>
#include <wx/sizer.h>
#include <util/timestamp.h>
#include <util/stringutil.h>
#include "envtab.h"
#include "reportexplorerid.h"
#include "reportexplorer.h"
#include "testdirdialog.h"
using namespace util::string;

namespace ods::gui {

wxBEGIN_EVENT_TABLE(EnvTab, wxPanel) // NOLINT
  EVT_LIST_ITEM_ACTIVATED(kIdEnvList, EnvTab::OnDoubleClick)
  EVT_CONTEXT_MENU(EnvTab::OnContextMenu)
wxEND_EVENT_TABLE()

EnvTab::EnvTab(wxWindow *parent)
: wxPanel(parent) {
  list = new wxListView(this, kIdEnvList, wxDefaultPosition,{800, 400}, wxLC_REPORT | wxLC_SINGLE_SEL );
  list->AppendColumn("Name", wxLIST_FORMAT_LEFT, 100);
  list->AppendColumn("Tests", wxLIST_FORMAT_LEFT, 75);
  list->AppendColumn("Files", wxLIST_FORMAT_LEFT, 75);
  list->AppendColumn("Progress Info", wxLIST_FORMAT_LEFT, 200);
  list->AppendColumn("Description", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(list,1, wxALL | wxEXPAND, 0);

  SetSizerAndFit(main_sizer);
}

void EnvTab::Update() {
  wxWindow::Update();
  RedrawEnvList();
}

void EnvTab::RedrawEnvList() {
  if (list == nullptr) {
    return;
  }
  auto selected_item = list->GetFirstSelected();
  wxString selected;
  if (selected_item >= 0) {
    selected = list->GetItemText(selected_item);
  }
  list->DeleteAllItems();
  selected_item = -1;
  const auto& app = wxGetApp();
  const auto& env_list = app.EnvList();
  long line = 0;
  for (const auto& itr : env_list) {
    const auto& env = itr.second;
    if (!env) {
      continue;
    }
    if (!selected.IsEmpty() && IEquals(env->Name(), selected.utf8_string())) {
      selected_item = line;
    }
    list->InsertItem(line, wxString::FromUTF8(env->Name()));
    list->SetItem(line,1, std::to_string(0)); // Nof Tests
    list->SetItem(line,2, std::to_string(0)); // Nof Files
    list->SetItem(line,3, "Idling"); // Progress Info
    list->SetItem(line,4, wxString::FromUTF8(env->Description())); // Progress Info
    ++line;
  }
  if (selected_item >= 0) {
    list->Select(selected_item, true);
  }
}

void EnvTab::OnEnvSelected(wxUpdateUIEvent &event) {
  event.Enable( list != nullptr && list->GetSelectedItemCount() == 1);
}

void EnvTab::OnEditEnv(wxCommandEvent &event) {
  if (list == nullptr || list->GetSelectedItemCount() != 1) {
    return;
  }
  auto item = list->GetFirstSelected();
  if (item < 0) {
    return;
  }
  const auto selected = list->GetItemText(item).utf8_string();
  auto& app = wxGetApp();
  auto* env = app.GetEnv(selected);
  if (env == nullptr) {
    return;
  }
  switch (env->Type()) {
    case EnvironmentType::kTypeTestDirectory: {
      auto* test_dir = dynamic_cast<detail::TestDirectory*>(env);
      if (test_dir == nullptr) {
        return;
      }
      TestDirDialog dialog(this, *test_dir);
      const auto ret = dialog.ShowModal();
      if (ret != wxID_OK) {
        return;
      }
      break;
    }
  }
  RedrawEnvList();
}

void EnvTab::OnDoubleClick(wxListEvent& event) {
  OnEditEnv(event);
}

void EnvTab::OnDeleteEnv(wxCommandEvent &event) {
  if (list == nullptr || list->GetSelectedItemCount() != 1) {
    return;
  }
  auto item = list->GetFirstSelected();
  if (item < 0) {
    return;
  }

  const auto selected = list->GetItemText(item).utf8_string();
  auto& app = wxGetApp();
  auto* env = app.GetEnv(selected);
  if (env == nullptr) {
    return;
  }
  std::ostringstream ask;
  ask << "Do you want to delete the selected environment?" << std::endl
      << "Environment: " << selected;
  auto ret = wxMessageBox(ask.str(), "Delete Environment Dialog",
                          wxYES_NO | wxCANCEL | wxNO_DEFAULT | wxICON_QUESTION, this);
  if (ret != wxYES) {
    return;
  }
  app.DeleteEnv(selected);
  RedrawEnvList();
}

void EnvTab::OnContextMenu(wxContextMenuEvent& event) {
  auto menu_new_env = new wxMenu;
  menu_new_env->Append(kIdNewEnvTestDir, "Test Directory");

  // ENV
  auto *menu_env = new wxMenu;
  menu_env->AppendSubMenu(menu_new_env, wxGetStockLabel(wxID_NEW));
  menu_env->Append(kIdEditEnv, wxGetStockLabel(wxID_EDIT));
  menu_env->Append(kIdDeleteEnv, wxGetStockLabel(wxID_DELETE));
  PopupMenu(menu_env);
}

}