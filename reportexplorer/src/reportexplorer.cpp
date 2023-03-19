/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <filesystem>
#include <algorithm>
#include <boost/process.hpp>
#include <boost/locale.hpp>

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/utils.h>

#include "util/logconfig.h"
#include "util/logstream.h"

#include "ods/ienvironmentcreator.h"
#include "ods/databaseguard.h"

#include "reportexplorer.h"
#include "mainframe.h"
#include "reportexplorerid.h"
#include "fetchvalue.h"

using namespace util::log;

namespace ods::gui {

wxIMPLEMENT_APP(ReportExplorer);

wxBEGIN_EVENT_TABLE(ReportExplorer, wxApp)
  EVT_UPDATE_UI(kIdOpenLogFile,ReportExplorer::OnUpdateOpenLogFile)
  EVT_MENU(kIdOpenLogFile, ReportExplorer::OnOpenLogFile)
  EVT_MENU(kIdModelFile, ReportExplorer::OnModelFile)
wxEND_EVENT_TABLE()


bool ReportExplorer::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }
  // Setup correct localization when formatting date and times
  boost::locale::generator gen;
  std::locale::global(gen(""));

  // Setup system basic configuration
  SetVendorDisplayName("Report Server");
  SetVendorName("ReportServer");

  SetAppDisplayName("Report Explorer");
  SetAppName("ReportExplorer");


  // Set up the log file.
  // The log file will be in c:/programdata/report_server/mdf_viewer.log
  auto& log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToFile);
  log_config.SubDir("report_server/log");
  log_config.BaseName("report_explorer");
  log_config.CreateDefaultLogger();
  LOG_DEBUG() << "Log File created. Path: " << log_config.GetLogFile();

  // Fetch the model file name. Needed if database is missing
  auto* app_config = wxConfig::Get();
  wxString temp_file;
  app_config->Read("/General/ModelFile",&temp_file, L"");
  model_filename_ = temp_file.ToStdString();

    // Create DB path if it's missing
  try {
    std::filesystem::path db_path(ProgramDataPath());
    db_path.append("report_server");
    db_path.append("db");
    std::filesystem::create_directories(db_path);
    db_path.append("reportexplorer.sqlite");
    db_filename_ = db_path.string();

    std::filesystem::path model_path(model_filename_);
    const auto model_exist = std::filesystem::exists(model_path);
    const auto db_exist = std::filesystem::exists(db_path);
    if (!db_exist && !model_exist) {
      // Ask for model path
      wxCommandEvent dummy;
      OnModelFile(dummy);
    }

  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to create the database path. Error: " << err.what();
    wxMessageBox("Failed to create the database path.\nMore information in the log file.");
  }
  LOG_DEBUG() << "DB Path: " << db_filename_;

  // Find the path to the 'notepad.exe'
  notepad_ = util::log::FindNotepad();
  LOG_DEBUG() << "Notepad Path: " << notepad_;

  temp_dir_ = std::make_unique<util::log::TempDir>("reportexplorer", true);

  wxPoint start_pos;
  app_config->Read("/MainWin/X",&start_pos.x, wxDefaultPosition.x);
  app_config->Read("/MainWin/Y",&start_pos.y, wxDefaultPosition.x);
  wxSize start_size;
  app_config->Read("/MainWin/XWidth",&start_size.x, 1200);
  app_config->Read("/MainWin/YWidth",&start_size.y, 800);
  bool maximized = false;
  app_config->Read("/MainWin/Max",&maximized, maximized);

  const auto init_system = InitSystem();
  if (!init_system) {
    wxMessageBox("Failed to initialize the system.\nMore information in the log file.");
  }
  LOG_DEBUG() << "Init system";

  LoadEnvList();
  LOG_DEBUG() << "Load environments";

  auto* frame = new MainFrame(GetAppDisplayName(), start_pos, start_size, maximized);
  frame->Show(true);

  return true;
}

int ReportExplorer::OnExit() {
  LOG_DEBUG() << "Closing application";
  for (auto& itr : env_list_) {
    LOG_DEBUG() << "Stopping environment. Environment: " << itr.first;
    if (itr.second) {
      itr.second->Stop();
    }
    LOG_DEBUG() << "Stopped environment. Environment: " << itr.first;
  }

  LOG_DEBUG() << "Saving environments";
  SaveEnvList();

  env_list_.clear();
  temp_dir_.reset();
  auto& log_config = LogConfig::Instance();
  log_config.DeleteLogChain();

  return wxApp::OnExit();
}

void ReportExplorer::OnOpenLogFile(wxCommandEvent& event) {
  auto& log_config = LogConfig::Instance();
  std::string logfile = log_config.GetLogFile();
  OpenFile(logfile);

}

void ReportExplorer::OnUpdateOpenLogFile(wxUpdateUIEvent &event) {
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

void ReportExplorer::OpenFile(const std::string& filename) const {
  if (!notepad_.empty()) {
    boost::process::spawn(notepad_, filename);
  }
}

EnvCreator& ReportExplorer::Creator() {
  return creator_;
}

void ReportExplorer::AddEnv(std::unique_ptr<IEnvironment> env) {
  if (env) {
    if (env->IsOk()) {
      env->Start();
      LOG_DEBUG() << "Started working thread. Environment: " << env->Name();
    }
    env_list_.insert({env->Name(), std::move(env)});
  }
}

const EnvironmentList &ReportExplorer::EnvList() const {
  return env_list_;
}

EnvironmentList &ReportExplorer::EnvList() {
  return env_list_;
}

void ReportExplorer::LoadEnvList() {
  auto* config = wxConfig::Get();
  if (!config) {
    LOG_ERROR() << "Failed to get the configuration for the application.";
    return;
  }

  const auto exist = config->HasGroup("EnvList");
  if (!exist) {
    LOG_DEBUG() << "No environment exists.";
    return;
  }

  const auto orig_path = config->GetPath();

  config->SetPath("/EnvList");
  wxString group;
  long index = 0;
  for (bool more = config->GetFirstGroup(group, index); more ; more = config->GetNextGroup(group, index)) {
    LOG_DEBUG() << "Environment: " << group;
    auto env = EnvCreator::CreateFromConfig(group.utf8_string());
    if (env) {
      AddEnv(std::move(env));
    }
  }

  config->SetPath(orig_path);
}

void ReportExplorer::SaveEnvList() {
  auto* config = wxConfig::Get();
  if (config == nullptr) {
    LOG_ERROR() << "Failed to get the configuration for the application.";
    return;
  }
  const auto& orig_path = config->GetPath();
  LOG_DEBUG() << "Original Path: " << orig_path;

  config->DeleteGroup("EnvList");
  for (const auto& itr : env_list_) {
    if (itr.first.empty() || !itr.second) {
      continue;
    }
    creator_.SaveToConfig(itr.second.get());
  }
  config->SetPath(orig_path);
}

IEnvironment *ReportExplorer::GetEnv(const std::string& name) {
  auto itr = env_list_.find(name);
  return itr == env_list_.end() ? nullptr : itr->second.get();
}

void ReportExplorer::DeleteEnv(const std::string &name) {
  auto itr = env_list_.find(name);
  if (itr != env_list_.end()) {
    env_list_.erase(itr);
  }
}

void ReportExplorer::GetTestBedList(NameIdMap &test_bed_list) {
  const auto* test_bed_table = model_.GetTableByName("TestBed");
  if (test_bed_table != nullptr) {
    DatabaseGuard db_lock(database_);
    IdNameMap temp_list;
    SqlFilter no_filter;
    database_.FetchNameMap(*test_bed_table, temp_list, no_filter);
    for (const auto& itr : temp_list) {
      test_bed_list.insert({itr.second, itr.first});
    }
  }
}

/**
 * Asks for path to the model file. Normally is this file delivered in the install kit but
 * sometimes the file is missing or the user wants to changes it.
 * @param event Not used
 */
void ReportExplorer::OnModelFile(wxCommandEvent &event) {
  auto* window = GetTopWindow(); // Note may return nullptr
  wxFileDialog dialog(window, ("Open ODS Model file"), "", "",
                      "ODS Model files (*.xml)|*.xml",
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  const auto ret = dialog.ShowModal();
  if (ret != wxID_OK) {
    return;
  }
  model_filename_ = dialog.GetPath();
  if (model_.IsEmpty()) {
    const auto read = model_.ReadModel(model_filename_);
    if (!read) {
      wxMessageBox("Failed to read in the model.\nMore information in the log file");
      return;
    }
  }

  auto* app_config = wxConfig::Get();
  wxString temp_file(model_filename_);
  app_config->Write("/General/ModelFile",temp_file);

}

bool ReportExplorer::InitSystem() {
  if (db_filename_.empty()) {
    LOG_ERROR() << "The database file name has not been set.";
    return false;
  }
  database_.FileName(db_filename_);

  // Check if we need to create the database
  bool need_create_db = false;
  try {
    std::filesystem::path db_path(db_filename_);
    need_create_db = !std::filesystem::exists(db_path);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Path error in database path. Error: " << err.what()
                << ", Path: " << db_filename_;
    return false;
  }

  if (need_create_db) {
    const auto create = CreateDb();
    if (!create) {
      LOG_ERROR() << "Failed to create the cache database. Path: " << db_filename_;
      return false;
    } else {
      LOG_DEBUG() << "Created a database. Database: " << db_filename_;
    }
  }

  // Now actual initialize the environment
  const auto init = InitDb();
  if (!init) {
    LOG_ERROR() << "Fail to initialize the database. Database: " << db_filename_;
    return false;
  } else {
    LOG_DEBUG() << "Read in model from the database. Database: " << db_filename_;
  }

  return true;
}

bool ReportExplorer::CreateDb() {
  if (model_filename_.empty()) {
    LOG_ERROR() << "No model file defined. Cannot create a database.";
    return false;
  }
  const auto read = model_.ReadModel(model_filename_);
  if (!read) {
    LOG_ERROR() << "Failed to read in the model. File: " << model_filename_;
    return false;
  }
  const auto create = database_.Create(model_);
  if (!create) {
    LOG_ERROR() << "Failed to create the cache database.";
    return false;
  }

  return true;
}

bool ReportExplorer::InitDb() {
  if (model_.IsEmpty()) {
    const auto read = database_.ReadModel(model_);
    if (!read) {
      LOG_ERROR() << "Failed to read in the ODS model from the database";
      return false;
    }
  }
  return true;
}

bool ReportExplorer::AddSelectedChannel(const IItem &item) {
  auto channel = std::make_unique<IItem>(item);
  const auto item_index = item.ItemId();

  const auto exist = std::ranges::any_of(selected_list_, [&] (const auto& chan) {
    return chan->ItemId() == item_index;
  });
  if (exist) {
    return true;
  }

  // Fetch name and referenced items from the database and add these properties
  const auto* meas_table = model_.GetBaseId(BaseId::AoMeasurement);
  const auto* quantity_table = model_.GetBaseId(BaseId::AoQuantity);
  const auto* unit_table = model_.GetBaseId(BaseId::AoUnit);
  const auto* file_table = model_.GetBaseId(BaseId::AoSubTest);
  const auto* test_table = model_.GetBaseId(BaseId::AoTest);
  if (meas_table == nullptr || quantity_table == nullptr ||
      unit_table == nullptr || file_table == nullptr ||
      test_table == nullptr) {
    return true;
  }

  const auto* meas_id_column = meas_table->GetColumnByBaseName("id");
  const auto* quantity_id_column = quantity_table->GetColumnByBaseName("id");
  const auto* unit_id_column = unit_table->GetColumnByBaseName("id");
  const auto* file_id_column = file_table->GetColumnByBaseName("id");
  const auto* test_id_column = test_table->GetColumnByBaseName("id");
  if (meas_id_column == nullptr || quantity_id_column == nullptr ||
      unit_id_column == nullptr || file_id_column == nullptr ||
      test_id_column == nullptr) {
    return true;
  }

  const auto meas_index = channel->BaseValue<int64_t>("measurement");
  const auto quantity_index = channel->BaseValue<int64_t>("quantity");
  const auto unit_index = channel->BaseValue<int64_t>("unit");

  ItemList meas_list;
  ItemList quantity_list;
  ItemList unit_list;
  ItemList file_list;
  ItemList test_list;
  DatabaseGuard db_lock(database_);
  try {
    if (meas_index > 0) {
      SqlFilter meas_filter;
      meas_filter.AddWhere(*meas_id_column, SqlCondition::Equal, meas_index);
      database_.FetchItemList(*meas_table, meas_list, meas_filter);
    }
    if (quantity_index > 0) {
      SqlFilter quantity_filter;
      quantity_filter.AddWhere(*quantity_id_column, SqlCondition::Equal, quantity_index);
      database_.FetchItemList(*quantity_table, quantity_list, quantity_filter);
    }
    if (unit_index > 0) {
      SqlFilter unit_filter;
      unit_filter.AddWhere(*unit_id_column, SqlCondition::Equal, unit_index);
      database_.FetchItemList(*unit_table, unit_list,unit_filter);
    }
    const auto file_index = meas_list.empty() ? 0 : meas_list.front()->BaseValue<int64_t>("test");
    if (file_index > 0) {
      SqlFilter file_filter;
      file_filter.AddWhere(*file_id_column, SqlCondition::Equal, file_index);
      database_.FetchItemList(*file_table, file_list, file_filter);
    }
    const auto test_index = file_list.empty() ? 0 : file_list.front()->BaseValue<int64_t>("parent_test");
    if (test_index > 0) {
      SqlFilter test_filter;
      test_filter.AddWhere(*test_id_column, SqlCondition::Equal, test_index);
      database_.FetchItemList(*test_table, test_list, test_filter);
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch channel info from the database. Error: " << err.what();
    db_lock.Rollback();
    return false;
  }
  if (!test_list.empty()) {
    const auto index = test_list.front()->ItemId();
    const auto name = test_list.front()->Name();
    channel->AppendAttribute({"TestIndex", "", index});
    channel->AppendAttribute({"TestName", "", name});
  }
  if (!file_list.empty()) {
    const auto index = file_list.front()->ItemId();
    const auto name = file_list.front()->Name();
    const auto test_file_index = file_list.front()->Value<int64_t>("TestFile"); // Index to physical file
    channel->AppendAttribute({"FileIndex", "", index});
    channel->AppendAttribute({"FileName", "", name});
    channel->AppendAttribute({"TestFile", "", test_file_index});
  }
  if (!meas_list.empty()) {
    const auto name = meas_list.front()->Name();
    const auto dg_index = meas_list.front()->Value<int64_t>("MeasIndex"); // Index to DG block
    channel->AppendAttribute({"MeasName", "", name});
    channel->AppendAttribute({"DgIndex", "", dg_index});
  }
  if (!unit_list.empty()) {
    const auto name = unit_list.front()->Name();
    channel->AppendAttribute({"UnitName", "", name});
  }
  if (!quantity_list.empty()) {
    const auto name = quantity_list.front()->Name();
    const auto desc = quantity_list.front()->BaseValue<std::string>("description");
    channel->AppendAttribute({"QuantityName", "", name});
    channel->AppendAttribute({"QuantityDesc", "", desc});
  }
  channel->AppendAttribute({"Selected", "", false});
  selected_list_.push_back(std::move(channel));
  return true;
}

void ReportExplorer::RemoveSelectedChannel(int64_t item_index) {
  auto itr = std::ranges::find_if(selected_list_, [&] (const auto& item) {
    return item->ItemId() == item_index;
  });

  if (itr != selected_list_.end()) {
    selected_list_.erase(itr);
  }
}

IItem *ReportExplorer::GetSelectedItem(int64_t index) {
  auto itr = std::ranges::find_if(selected_list_, [&] (const auto& item) {
    return item->ItemId() == index;
  });
  return itr == selected_list_.end() ? nullptr : itr->get();
}

std::string ReportExplorer::FetchFile(int64_t index) {
  const auto* table = model_.GetTableByName("TestFile");
  const auto* id_column = table == nullptr ? nullptr : table->GetColumnByBaseName("id");
  if (table == nullptr || id_column == nullptr || index <= 0) {
    return {};
  }
  SqlFilter filter;
  filter.AddWhere(*id_column, SqlCondition::Equal, index);
  ItemList file_list;
  DatabaseGuard db_lock(database_);
  try {
    database_.FetchItemList(*table, file_list,filter);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch a file. Index: " << index;
    db_lock.Rollback();
    return {};
  }
  return file_list.empty() ? std::string() : file_list[0]->BaseValue<std::string>("ao_location");
}

void ReportExplorer::PlotSelectedItem(bool save_report) {
  FetchValue fetcher;

  // 1. First add all selected items
  for (const auto& itr : selected_list_) {
    if (!itr) {
      continue;
    }
    const auto sel = itr->Value<std::string>("Selected");
    if (sel == "0") {
      continue;
    }
    fetcher.AddItem(*itr);
  }
  // 2. Read in all values
  fetcher.ReadValues();

  // 3. Select temporary storage or permanent for the CSV and plot file
  auto& csv = plotter_.CsvFile();
  csv.Reset();  // Note closes any opened files
  std::string base_name = plotter_.Name();
  if (base_name.empty()) {
    base_name = "temp";
  }

  std::string csv_file;
  if (save_report) {
    csv_file = plotter_.CsvFileName();
    try {
      const auto exist = std::filesystem::exists(csv_file);
      if (exist) {
        std::ostringstream ask;
        ask << "The file exist. Do you want to replace it?" << std::endl
            << "CSV File: " << csv_file;
        const auto ret = wxMessageBox(ask.str(), L"Overwrite CSV File Dialog",
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION | wxNO_DEFAULT,
                         wxGetActiveWindow());
        if (ret != wxYES) {
          return;
        }
      }
    } catch (const std::exception& err) {
      LOG_ERROR() << "Failed to test if CSV file existed. Error: " << err.what()
                  << ", CSV File Name: " << csv_file;
      return;
    }
  } else {
    csv_file = temp_dir_->TempFile(base_name, "csv", true);
  }
  csv.OpenFile(csv_file);
  fetcher.ExportToCsv(csv);
  csv.CloseFile();

  // If any of the columns have all invalid values then gnuplot fails.
  bool has_invalid_column = false;
  std::ostringstream invalid;
  invalid << "The following channels are invalid and cannot be plotted!" << std::endl;
  for (size_t column = 0; column < csv.Columns(); ++column) {
    if (!csv.IsColumnValid(column)) {
      has_invalid_column = true;
      invalid << "Channel: " << csv.Name(column) << std::endl;
    }
  }
  if (has_invalid_column) {
    wxMessageBox(invalid.str());
    return;
  }
  plotter_.SaveScript();
  plotter_.Show();
}

}


