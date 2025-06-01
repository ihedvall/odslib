//
// Created by ihedv on 2025-02-05.
//

#include "mainframe.h"

#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <wx/menu.h>
#include <wx/stockitem.h>
#include <wx/docview.h>
#include "wx/ribbon/buttonbar.h"
#include "wx/ribbon/gallery.h"
#include "wx/ribbon/toolbar.h"
#include "wx/artprov.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/checkbox.h"

#include "globalnameid.h"

#include "img/ribbon.xpm"
#include "img/align_center.xpm"
#include "img/align_left.xpm"
#include "img/align_right.xpm"

namespace ods {


wxBEGIN_EVENT_TABLE(MainFrame, wxDocParentFrame) // NOLINT
    EVT_CLOSE(MainFrame::OnClose)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& start_pos, const wxSize& start_size, bool maximized)
    : wxDocParentFrame(wxDocManager::GetDocumentManager(), nullptr, wxID_ANY, title, start_pos, start_size) {

  SetIcon(wxIcon("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
  wxWindow::SetName("GlobalNameTopWindow");
  wxTopLevelWindowMSW::Maximize(maximized);
  wxWindow::DragAcceptFiles(true);

  MakeMenu();
  MakeStatusBar();

}

void MainFrame::MakeMenu() {

  // DATABASE
  auto *menu_db = new wxMenu;
  menu_db->Append(wxID_NEW);
  menu_db->Append(wxID_OPEN);
  menu_db->Append(wxID_CLOSE);
  menu_db->AppendSeparator();
  menu_db->Append(wxID_EXIT);

  if (auto *doc_manager = wxDocManager::GetDocumentManager();
      doc_manager != nullptr ) {
    doc_manager->FileHistoryUseMenu(menu_db);
    if (auto *app_config = wxConfig::Get();
        app_config != nullptr ) {
      doc_manager->FileHistoryLoad(*app_config);
    }
  }

  // ABOUT
  auto *menu_about = new wxMenu;
  menu_about->Append(kIdOpenLogFile, L"Open Log File");
  menu_about->AppendSeparator();
  menu_about->Append(wxID_ABOUT, wxGetStockLabel(wxID_ABOUT));

  auto *menu_bar = new wxMenuBar;
  menu_bar->Append(menu_db,"&Database");

  menu_bar->Append(menu_about, wxGetStockLabel(wxID_HELP));
  wxFrameBase::SetMenuBar(menu_bar);

}

void MainFrame::MakeStatusBar() {
  if (auto* old_status_bar = GetStatusBar();
      old_status_bar != nullptr) {
    SetStatusBar(nullptr);
    delete old_status_bar;
  }
  CreateStatusBar(1, wxSTB_DEFAULT_STYLE, wxID_ANY, "StatusBar");
}

void MainFrame::OnClose(wxCloseEvent &event) {
  // If the window is minimized, do not save the last position.
  if (!IsIconized()) {
    bool maximized = IsMaximized();
    wxPoint end_pos = GetPosition();
    wxSize end_size = GetSize();

    if (auto* app_config = wxConfig::Get();
        app_config != nullptr ) {
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
  }
  event.Skip(true);
}

void MainFrame::OnAbout(wxCommandEvent&) {
  wxAboutDialogInfo info;
  info.SetName("Global Name Configuration Tool");
  info.SetVersion("1.0");
  info.SetDescription("Configuration Tool for basic I/O and global naming.");

  wxArrayString devs;
  devs.push_back("Ingemar Hedvall");
  info.SetDevelopers(devs);

  info.SetCopyright("(C) 2025 Ingemar Hedvall");
  info.SetLicense("MIT License (https://opensource.org/licenses/MIT)\n"
                  "Copyright 2025 Ingemar Hedvall\n"
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

} // ods