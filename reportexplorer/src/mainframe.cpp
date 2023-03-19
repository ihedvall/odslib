/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <wx/choicdlg.h>
#include "mainframe.h"
#include "reportexplorerid.h"
#include "reportexplorer.h"
#include "envtab.h"
#include "measurementtab.h"
#include "plottab.h"
#include "testdirdialog.h"

namespace {
  constexpr int kPageEnvironment = 0;
  constexpr int kPageSelection = 1;
  constexpr int kPagePlot = 2;
}

namespace ods::gui {

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame) // NOLINT
  EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
  EVT_MENU(wxID_EXIT, MainFrame::OnExit)
  EVT_NOTEBOOK_PAGE_CHANGED(kIdNotebook, MainFrame::OnPageChange)

  EVT_MENU(kIdNewEnvTestDir, MainFrame::OnNewEnvTestDir)
  EVT_UPDATE_UI(kIdEditEnv, MainFrame::OnEnvSelected)
  EVT_MENU(kIdEditEnv, MainFrame::OnEditEnv)
  EVT_UPDATE_UI(kIdDeleteEnv, MainFrame::OnEnvSelected)
  EVT_MENU(kIdDeleteEnv, MainFrame::OnDeleteEnv)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& start_pos, const wxSize& start_size, bool maximized)
    : wxFrame(nullptr, wxID_ANY, title, start_pos, start_size),
    image_list_(32,32,false,6) {
  wxTopLevelWindowMSW::Maximize(maximized);
  SetIcon(wxIcon("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
  image_list_.Add(wxBitmap("NOTEBOOK_LIST", wxBITMAP_TYPE_BMP_RESOURCE));

  // FILE
  auto *menu_file = new wxMenu;
  menu_file->Append(wxID_EXIT);

  auto menu_new_env = new wxMenu;
  menu_new_env->Append(kIdNewEnvTestDir, "Test Directory");

  // ENV
  auto *menu_env = new wxMenu;
  menu_env->AppendSubMenu(menu_new_env, wxGetStockLabel(wxID_NEW));
  menu_env->Append(kIdEditEnv, wxGetStockLabel(wxID_EDIT));
  menu_env->Append(kIdDeleteEnv, wxGetStockLabel(wxID_DELETE));

  // ABOUT
  auto *menu_about = new wxMenu;
  menu_about->Append(kIdOpenLogFile, L"Open Log File");
  menu_about->AppendSeparator();
  menu_about->Append(kIdModelFile, L"Select Model File");
  menu_about->AppendSeparator();
  menu_about->Append(wxID_ABOUT, wxGetStockLabel(wxID_ABOUT));

  auto *menu_bar = new wxMenuBar;
  menu_bar->Append(menu_file, wxGetStockLabel(wxID_FILE));
  menu_bar->Append(menu_env, L"Environment");
  menu_bar->Append(menu_about, wxGetStockLabel(wxID_HELP));
  wxFrameBase::SetMenuBar(menu_bar);

  notebook_ = new wxNotebook(this, kIdNotebook);
  notebook_->SetImageList(&image_list_);

  auto* env_tab = new EnvTab(notebook_);
  auto* select_tab = new MeasurementTab(notebook_);
  auto* plot_tab = new PlotTab(notebook_);
  notebook_->AddPage(env_tab, L"Environment", true,3);
  notebook_->AddPage(select_tab, L"Selection", true,4);
  notebook_->AddPage(plot_tab, L"Plot", true,5);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(notebook_, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 4);
  SetSizerAndFit(main_sizer);

  auto& app = wxGetApp();
  if (app.EnvList().empty()) {
    notebook_->SetSelection(kPageEnvironment);
    notebook_->SetPageImage(kPageEnvironment,0);
  } else {
    notebook_->SetSelection(kPageSelection);
    notebook_->SetPageImage(kPageSelection,1);
  }
}

void MainFrame::OnExit(wxCommandEvent &event) {

  // If the window is minimized. Do not save as last position

  if (!IsIconized()) {
    bool maximized = IsMaximized();
    wxPoint end_pos = GetPosition();
    wxSize end_size = GetSize();
    auto* app_config = wxConfig::Get();

    if (maximized) {
      app_config->Write("/MainWin/Max",maximized);
    } else {
      app_config->Write("/MainWin/X", end_pos.x);
      app_config->Write("/MainWin/Y", end_pos.y);
      app_config->Write("/MainWin/XWidth", end_size.x);
      app_config->Write("/MainWin/YWidth", end_size.y);
      app_config->Write("/MainWin/Max", maximized);
    }
  }
  Close(true);
}

void MainFrame::OnAbout(wxCommandEvent&) {

  wxAboutDialogInfo info;
  info.SetName("Report Explorer");
  info.SetVersion("1.0");
  info.SetDescription("Report Explorer.");

  wxArrayString devs;
  devs.push_back("Ingemar Hedvall");
  info.SetDevelopers(devs);

  info.SetCopyright("(C) 2022 Ingemar Hedvall");
  info.SetLicense("MIT License (https://opensource.org/licenses/MIT)\n"
      "Copyright 2022 Ingemar Hedvall\n"
      "\n"
      "Permission is hereby granted, free of charge, to any person obtaining a copy of this\n"
      "software and associated documentation files (the \"Software\"),\n"
      "to deal in the Software without restriction, including without limitation the rights to use, copy,\n"
      "modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,\n"
      "and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n"
      "\n"
      "The above copyright notice and this permission notice shall be included in all copies or substantial\n"
      "portions of the Software.\n"
      "\n"
      "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,\n"
      "INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR\n"
      "PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,\n"
      "DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR\n"
      "IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."
  );
  wxAboutBox(info);
}

void MainFrame::OnPageChange(wxBookCtrlEvent &event) {
  if (notebook_ == nullptr) {
    return;
  }
  const auto old_page = event.GetOldSelection();
  const auto page = event.GetSelection();
  auto* window = notebook_->GetPage(page);
  if (old_page != wxNOT_FOUND) {
    notebook_->SetPageImage(old_page, old_page + 3);
  }
  if (page != wxNOT_FOUND) {
    notebook_->SetPageImage(page, page);
  }
  if (window != nullptr) {
    window->Update();
  }
}

void MainFrame::Update() {
  wxWindow::Update();
  const size_t count = notebook_->GetPageCount();
  for (size_t page = 0; page < count; ++page) {
    auto* tab = notebook_->GetPage(page);
    if (tab != nullptr) {
      tab->Update();
    }
  }
}
void MainFrame::RedrawSelectedList() {
  auto* tab = dynamic_cast<MeasurementTab*>(notebook_->GetPage(kPageSelection));
  if (tab != nullptr) {
    tab->RedrawSelectedList();
  }
}

size_t MainFrame::GetSelectedCount() const {
  auto* tab = dynamic_cast<MeasurementTab*>(notebook_->GetPage(kPageSelection));
  return tab == nullptr ? 0 : tab->GetSelectedCount();
}

void MainFrame::RemoveSelected() {
  auto* tab = dynamic_cast<MeasurementTab*>(notebook_->GetPage(kPageSelection));
  if (tab != nullptr) {
    tab->RemoveSelected();
  }
}
void MainFrame::OnNewEnvTestDir(wxCommandEvent &event) {
  auto& app = wxGetApp();
  auto& creator = app.Creator();

  auto temp = creator.CreateEnvironment(EnvironmentType::kTypeTestDirectory);
  auto* test_dir = dynamic_cast<detail::TestDirectory*>(temp.get());
  if (test_dir == nullptr) {
    return;
  }
  TestDirDialog dialog(this, *test_dir);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  const auto init = test_dir->Init();
  if (!init) {
    wxMessageBox("Failed to initialize the environment!.\nMore information in the log file.",
                 "Init Failure Dialog", wxOK | wxICON_ERROR, this);
  }
  app.AddEnv(std::move(temp));
  Update();
}

void MainFrame::OnEnvSelected(wxUpdateUIEvent &event) {
  if (notebook_ == nullptr || notebook_->GetSelection() != kPageEnvironment) {
    event.Enable(false);
    return;
  }
  auto* env_page = dynamic_cast<EnvTab*>(notebook_->GetCurrentPage());
  if (env_page != nullptr) {
    env_page->OnEnvSelected(event);
  } else {
    event.Enable(false);
  }
}

void MainFrame::OnEditEnv(wxCommandEvent &event) {
  auto* env_page = dynamic_cast<EnvTab*>(notebook_->GetCurrentPage());
  if (env_page != nullptr) {
    env_page->OnEditEnv(event);
  }
}

void MainFrame::OnDeleteEnv(wxCommandEvent &event) {
  auto* env_page = dynamic_cast<EnvTab*>(notebook_->GetCurrentPage());
  if (env_page != nullptr) {
    env_page->OnDeleteEnv(event);
  }
}


}