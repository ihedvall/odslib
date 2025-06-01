/*
 * Copyright 2023 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "globalnameapp.h"

#include <filesystem>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/locale.hpp>

#include <util/logconfig.h>
#include <util/logstream.h>

#include <wx/wx.h>
#include <wx/docview.h>
#include <wx/config.h>
#include <wx/utils.h>

#include "globalnameid.h"
#include "globalnamedoc.h"
#include "globalnameview.h"
#include "mainframe.h"

using namespace util::log;
using namespace std::filesystem;

namespace {
  boost::asio::io_context kIoContext;
} // End namespace

namespace ods {

wxIMPLEMENT_APP(GlobalNameApp);

wxBEGIN_EVENT_TABLE(GlobalNameApp, wxApp)
        EVT_UPDATE_UI(kIdOpenLogFile,GlobalNameApp::OnUpdateOpenLogFile)
        EVT_MENU(kIdOpenLogFile, GlobalNameApp::OnOpenLogFile)
wxEND_EVENT_TABLE()

bool GlobalNameApp::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }
  // Setup correct localization when formatting date and times
  boost::locale::generator gen;
  std::locale::global(gen(""));

  // Setup system basic configuration
  SetVendorDisplayName("Global Name Configuration Tool");
  SetVendorName("IH Development");
  SetAppName("GlobalName");
  SetAppDisplayName("Global Name Configuration Tool");

  // Set up the log file.
  // The log file will be in %TEMP%/report_server/mdf_viewer.log
  auto &log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToFile);
  log_config.SubDir("ih_develop/log"); // <Program Data>/ih_develop/log/<log files>
  log_config.BaseName("global_name"); // Logfile stem name
  log_config.CreateDefaultLogger();
  LOG_DEBUG() << "Log File created. Path: " << log_config.GetLogFile();

  // Find the path to the 'notepad.exe' that is used for
  // showing log file contents
  notepad_ = util::log::FindNotepad();

  wxPoint start_pos;
  wxSize start_size;
  bool maximized = false;
  if (auto *app_config = wxConfig::Get();
      app_config != nullptr) {
    app_config->Read("/MainWin/X", &start_pos.x, wxDefaultPosition.x);
    app_config->Read("/MainWin/Y", &start_pos.y, wxDefaultPosition.x);
    app_config->Read("/MainWin/XWidth", &start_size.x, 1200);
    app_config->Read("/MainWin/YWidth", &start_size.y, 800);
    app_config->Read("/MainWin/Max", &maximized, maximized);
  }

  if (auto* doc_manager = new wxDocManager;
      doc_manager != nullptr ) {
    new wxDocTemplate(doc_manager, "Global Name Database", "*.sqlite", "",
                      "sqlite", "GlobalName", "Global Name Configuration Tool",
                      wxCLASSINFO(GlobalNameDoc), wxCLASSINFO(GlobalNameView));
    doc_manager->SetMaxDocsOpen(1); // Single database open
  }

  if (auto* frame = new MainFrame(GetAppDisplayName(), start_pos, start_size, maximized);
      frame != nullptr) {
    frame->Show(true);
  }

  return true;
}

int GlobalNameApp::OnExit() {
  LOG_INFO() << "Closing application";
  if (auto* app_config = wxConfig::Get();
      app_config != nullptr ) {
    if (auto *doc_manager = wxDocManager::GetDocumentManager();
        doc_manager != nullptr ) {
      doc_manager->FileHistorySave(*app_config);
      delete doc_manager;
    }
  }
  LOG_INFO() << "Saved file history.";

  auto& log_config = LogConfig::Instance();
  log_config.DeleteLogChain();

  return wxApp::OnExit();
}

void GlobalNameApp::OnOpenLogFile(wxCommandEvent& event) {
  const LogConfig& log_config = LogConfig::Instance();
  std::string logfile = log_config.GetLogFile();
  OpenFile(logfile);
}

void GlobalNameApp::OpenFile(const std::string& filename) const {
  if (!notepad_.empty()) {
    boost::process::process proc(kIoContext, notepad_, {filename});
    proc.detach();
  }
}

void GlobalNameApp::OnUpdateOpenLogFile(wxUpdateUIEvent &event) {
  if (notepad_.empty()) {
    event.Enable(false);
    return;
  }

  const auto& log_config = LogConfig::Instance();

  try {
    path log_file(log_config.GetLogFile());
    const bool exist = exists(log_file);
    event.Enable(exist);
  } catch (const std::exception&) {
    event.Enable(false);
  }
}

} // ods