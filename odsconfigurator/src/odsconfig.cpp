/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <filesystem>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <wx/wx.h>
#include <wx/docview.h>
#include <wx/config.h>
#include <wx/utils.h>

#include "util/logconfig.h"
#include "util/logstream.h"

#include "odsconfig.h"
#include "mainframe.h"
#include "odsdocument.h"
#include "odsview.h"
#include "odsconfigid.h"

using namespace util::log;

namespace ods::gui {

wxIMPLEMENT_APP(OdsConfig);

wxBEGIN_EVENT_TABLE(OdsConfig, wxApp)
  EVT_UPDATE_UI(kIdOpenLogFile,OdsConfig::OnUpdateOpenLogFile)
  EVT_MENU(kIdOpenLogFile, OdsConfig::OnOpenLogFile)
wxEND_EVENT_TABLE()

bool OdsConfig::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }
  // Setup correct localization when formatting date and times
  boost::locale::generator gen;
  std::locale::global(gen(""));

  // Setup system basic configuration
  SetVendorDisplayName("ODS Configurator");
  SetVendorName("ReportServer");
  SetAppName("OdsConfig");
  SetAppDisplayName("ODS Configurator");

  // Set up the log file.
  // The log file will be in %TEMP%/report_server/mdf_viewer.log
  auto& log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToFile);
  log_config.SubDir("report_server/log");
  log_config.BaseName("ods_config");
  log_config.CreateDefaultLogger();
  LOG_DEBUG() << "Log File created. Path: " << log_config.GetLogFile();



  // Find the path to the 'notepad.exe'
  notepad_ = util::log::FindNotepad();

  auto* app_config = wxConfig::Get();
  wxPoint start_pos;
  app_config->Read("/MainWin/X",&start_pos.x, wxDefaultPosition.x);
  app_config->Read("/MainWin/Y",&start_pos.y, wxDefaultPosition.x);
  wxSize start_size;
  app_config->Read("/MainWin/XWidth",&start_size.x, 1200);
  app_config->Read("/MainWin/YWidth",&start_size.y, 800);
  bool maximized = false;
  app_config->Read("/MainWin/Max",&maximized, maximized);

  auto* doc_manager = new wxDocManager;
  new wxDocTemplate(doc_manager, "ODS Configuration Files","*.xml","",
                                         "xml","OdsConfig","ODS Configurator",
                                         wxCLASSINFO(OdsDocument), wxCLASSINFO(OdsView));

  auto* frame = new MainFrame(GetAppDisplayName(), start_pos, start_size, maximized);

  frame->Show(true);

  return true;
}

int OdsConfig::OnExit() {
  LOG_INFO() << "Closing application";
  auto* app_config = wxConfig::Get();
  auto* doc_manager = wxDocManager::GetDocumentManager();
  doc_manager->FileHistorySave(*app_config);
  delete doc_manager;
  LOG_INFO() << "Saved file history.";

  auto& log_config = LogConfig::Instance();
  log_config.DeleteLogChain();
  return wxApp::OnExit();
}

void OdsConfig::OnOpenLogFile(wxCommandEvent& event) {
  auto& log_config = LogConfig::Instance();
  std::string logfile = log_config.GetLogFile();
  OpenFile(logfile);

}

void OdsConfig::OnUpdateOpenLogFile(wxUpdateUIEvent &event) {
  if (notepad_.empty()) {
    event.Enable(false);
    return;
  }

  auto& log_config = LogConfig::Instance();
  std::string logfile = log_config.GetLogFile();
  try {
    std::filesystem::path p(logfile);
    const bool exist = std::filesystem::exists(p);
    event.Enable(exist);
  } catch (const std::exception&) {
    event.Enable(false);
  }
}

void OdsConfig::OpenFile(const std::string& filename) const {
  if (!notepad_.empty()) {
    boost::process::spawn(notepad_, filename);
  }
}

}


