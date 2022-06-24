/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <string>
#include <filesystem>

#include <wx/config.h>
#include <wx/aboutdlg.h>

#include <util/logstream.h>
#include <ods/atfxfile.h>
#include "../../odslib/src/sqlitedatabase.h"
#include "mainframe.h"
#include "odsconfigid.h"
#include "odsdocument.h"


using namespace util::log;

namespace ods::gui {

wxBEGIN_EVENT_TABLE(MainFrame, wxDocMDIParentFrame) // NOLINT
  EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
  EVT_CLOSE(MainFrame::OnClose)
  EVT_UPDATE_UI(wxID_SAVE, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(wxID_SAVEAS, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(wxID_CLOSE, MainFrame::OnUpdateNoDoc)

  EVT_UPDATE_UI(kIdAddTable, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdEditTable, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdDeleteTable, MainFrame::OnUpdateNoDoc)

  EVT_UPDATE_UI(kIdAddColumn, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdEditColumn, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdDeleteColumn, MainFrame::OnUpdateNoDoc)

  EVT_UPDATE_UI(kIdAddEnum, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdEditEnum, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdDeleteEnum, MainFrame::OnUpdateNoDoc)

  EVT_UPDATE_UI(kIdAddEnumItem, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdEditEnumItem, MainFrame::OnUpdateNoDoc)
  EVT_UPDATE_UI(kIdDeleteEnumItem, MainFrame::OnUpdateNoDoc)

  EVT_MENU(kIdImportFile, MainFrame::OnImportAtfx)
  EVT_MENU(kIdImportSqlite, MainFrame::OnImportSqlite)

  EVT_UPDATE_UI(kIdCreateDbSqlite, MainFrame::OnUpdateAnyDoc)
  EVT_MENU(kIdCreateDbSqlite, MainFrame::OnCreateDbSqlite)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& start_pos, const wxSize& start_size, bool maximized)
    : wxDocMDIParentFrame(wxDocManager::GetDocumentManager(), nullptr, wxID_ANY, title, start_pos, start_size) {

  SetIcon(wxIcon("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
  wxWindow::SetName("OdsTopWindow");
  wxTopLevelWindowMSW::Maximize(maximized);
  wxWindow::DragAcceptFiles(true);

  auto *app_config = wxConfig::Get();
  auto *doc_manager = wxDocManager::GetDocumentManager();

  auto* menu_import = new wxMenu;
  menu_import->Append(kIdImportFile, "ATFX File");
  menu_import->Append(kIdImportSqlite, "SQLite Database");

  auto* menu_create = new wxMenu;
  menu_create->Append(kIdCreateDbSqlite, "SQLite Database");

  // FILE
  auto *menu_file = new wxMenu;
  menu_file->Append(wxID_NEW);
  menu_file->Append(wxID_OPEN);
  menu_file->AppendSeparator();
  menu_file->Append(wxID_SAVE);
  menu_file->Append(wxID_SAVEAS);
  menu_file->Append(wxID_CLOSE);
  menu_file->AppendSeparator();
  menu_file->AppendSubMenu(menu_import, "Import...");
  menu_file->AppendSubMenu(menu_create, "Create...");
  menu_file->AppendSeparator();
  menu_file->Append(wxID_EXIT);

  doc_manager->FileHistoryUseMenu(menu_file);
  doc_manager->FileHistoryLoad(*app_config);

  // TABLE
  auto *menu_table = new wxMenu;
  menu_table->Append(kIdAddTable,wxGetStockLabel(wxID_ADD));
  menu_table->Append(kIdEditTable, wxGetStockLabel(wxID_EDIT));
  menu_table->Append(kIdDeleteTable, wxGetStockLabel(wxID_DELETE));

  // COLUMN
  auto *menu_column = new wxMenu;
  menu_column->Append(kIdAddColumn,wxGetStockLabel(wxID_ADD));
  menu_column->Append(kIdEditColumn, wxGetStockLabel(wxID_EDIT));
  menu_column->Append(kIdDeleteColumn, wxGetStockLabel(wxID_DELETE));

  // ENUM
  auto *menu_enum = new wxMenu;
  menu_enum->Append(kIdAddEnum,wxGetStockLabel(wxID_ADD));
  menu_enum->Append(kIdEditEnum, wxGetStockLabel(wxID_EDIT));
  menu_enum->Append(kIdDeleteEnum, wxGetStockLabel(wxID_DELETE));

  // ITEM
  auto *menu_item = new wxMenu;
  menu_item->Append(kIdAddEnumItem,wxGetStockLabel(wxID_ADD));
  menu_item->Append(kIdEditEnumItem, wxGetStockLabel(wxID_EDIT));
  menu_item->Append(kIdDeleteEnumItem, wxGetStockLabel(wxID_DELETE));

  // ABOUT
  auto *menu_about = new wxMenu;
  menu_about->Append(kIdOpenLogFile, L"Open Log File");
  menu_about->AppendSeparator();
  menu_about->Append(wxID_ABOUT, wxGetStockLabel(wxID_ABOUT));

  auto *menu_bar = new wxMenuBar;
  menu_bar->Append(menu_file, wxGetStockLabel(wxID_FILE));
  menu_bar->Append(menu_table, L"Table");
  menu_bar->Append(menu_column, L"Column");
  menu_bar->Append(menu_enum, L"Enumerate");
  menu_bar->Append(menu_item, L"Enumerate Item");
  menu_bar->Append(menu_about, wxGetStockLabel(wxID_HELP));
  wxFrameBase::SetMenuBar(menu_bar);
}

void MainFrame::OnClose(wxCloseEvent &event) {

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
  event.Skip(true);
}

void MainFrame::OnAbout(wxCommandEvent&) {
  wxAboutDialogInfo info;
  info.SetName("ODS Configurator");
  info.SetVersion("1.0");
  info.SetDescription("ODS Database Configuration Tool.");

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


void MainFrame::OnUpdateNoDoc(wxUpdateUIEvent &event) { //NOLINT
  auto* man = wxDocManager::GetDocumentManager();
  auto* doc = man != nullptr ? man->GetCurrentDocument() : nullptr;
  if (doc == nullptr) {
    event.Enable(false);
    event.Skip(false);
  } else {
    event.Skip(true);
  }
}

void MainFrame::OnUpdateAnyDoc(wxUpdateUIEvent &event) { //NOLINT
  auto* man = wxDocManager::GetDocumentManager();
  auto* doc = man != nullptr ? man->GetCurrentDocument() : nullptr;
  event.Enable(doc != nullptr);
}

void MainFrame::OnImportAtfx(wxCommandEvent &event) {

  wxFileDialog dialog(this, "Select ODS Source", "", "",
                 "ATFX files (*.atfx)|*.atfx|All files (*.*)|*.*",
                 wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }

  const auto filename = dialog.GetPath().ToStdString();

  AtfxFile atfx_file;
  atfx_file.FileName(filename);
  const auto import = atfx_file.Import();
  if (!import) {
    std::ostringstream err;
    err << "Import of the file failed!" << std::endl;
    err << "More information in the log file." << std::endl;
    err << "File: " << filename;
    wxMessageBox(err.str(), L"ATFX File error", wxOK | wxCENTRE | wxICON_ERROR,this);
    return;
  }
  auto* doc_manager = wxDocManager::GetDocumentManager();
  if (doc_manager == nullptr) {
    LOG_ERROR() << "Failed to get the document manager.";
    return;
  }
  auto* doc = doc_manager->CreateNewDocument();
  if (doc == nullptr) {
    LOG_ERROR() << "Failed to create a new document.";
    return;
  }
  wxString title;
  try {
    const std::filesystem::path full_path(filename);
    title = full_path.filename().wstring();
  } catch (const std::exception& error) {
    LOG_ERROR() << "Invalid path. Error: " << error.what() << ", File: " << filename;
  }
  auto* ods_doc = wxDynamicCast(doc, OdsDocument); // NOLINT
  if (ods_doc == nullptr) {
    LOG_ERROR() << "Failed to convert the document.";
    return;
  }
  ods_doc->SetTitle(title);
  ods_doc->SetModel(atfx_file.Model());
  ods_doc->UpdateAllViews();

}

void MainFrame::OnImportSqlite(wxCommandEvent &event) {

  wxFileDialog dialog(this, "Select ODS Source", "", "",
                      "SQLite Database (*.sqlite)|*.sqlite|All files (*.*)|*.*",
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }

  const auto filename = dialog.GetPath().ToStdString();

  detail::SqliteDatabase database;
  database.FileName(filename);
  IModel model;
  const auto import = database.ReadModel(model);
  if (!import) {
    std::ostringstream err;
    err << "Import of the model failed!" << std::endl;
    err << "More information in the log file." << std::endl;
    err << "Database: " << filename;
    wxMessageBox(err.str(), L"Import DB Error", wxOK | wxCENTRE | wxICON_ERROR,this);
    return;
  }

  auto* doc_manager = wxDocManager::GetDocumentManager();
  if (doc_manager == nullptr) {
    LOG_ERROR() << "Failed to get the document manager.";
    return;
  }
  auto* doc = doc_manager->CreateNewDocument();
  if (doc == nullptr) {
    LOG_ERROR() << "Failed to create a new document.";
    return;
  }
  wxString title;
  try {
    const std::filesystem::path full_path(filename);
    title = full_path.filename().wstring();
  } catch (const std::exception& error) {
    LOG_ERROR() << "Invalid path. Error: " << error.what() << ", File: " << filename;
  }
  auto* ods_doc = wxDynamicCast(doc, OdsDocument); // NOLINT
  if (ods_doc == nullptr) {
    LOG_ERROR() << "Failed to convert the document.";
    return;
  }
  ods_doc->SetTitle(title);
  ods_doc->SetModel(model);
  ods_doc->UpdateAllViews();

}

void MainFrame::OnCreateDbSqlite(wxCommandEvent &event) {
  auto* man = wxDocManager::GetDocumentManager();
  auto* doc = man != nullptr ? man->GetCurrentDocument() : nullptr;
  if (doc == nullptr) {
    return;
  }
  auto* document = wxDynamicCast(doc, OdsDocument); //NOLINT
  if (document == nullptr) {
    return;
  }
  auto& model = document->GetModel();
  std::ostringstream temp;
  temp << model.Name() << ".sqlite";

  wxFileDialog dialog(this, "Select DB File", "", temp.str(),
                      "SQLite Database (*.sqlite)|*.sqlite|All files (*.*)|*.*",
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }

  const auto filename = dialog.GetPath().ToStdString();

  detail::SqliteDatabase database;
  database.FileName(filename);
  BackupFiles(filename);
  const auto create = database.Create(model);
  if (!create) {
    std::ostringstream err;
    err << "Create of the database failed!" << std::endl;
    err << "More information in the log file." << std::endl;
    err << "Database: " << filename;
    wxMessageBox(err.str(), L"Create DB Error", wxOK | wxCENTRE | wxICON_ERROR,this);
    return;
  }
  document->UpdateAllViews();
}

}