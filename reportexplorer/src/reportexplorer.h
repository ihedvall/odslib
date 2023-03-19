/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <map>
#include <memory>
#include <wx/wx.h>
#include <util/tempdir.h>
#include <util/gnuplot.h>
#include <ods/iitem.h>
#include <ods/ienvironment.h>
#include "sqlitedatabase.h"
#include "envcreator.h"


namespace ods::gui {

class ReportExplorer : public wxApp {
 public:

  bool OnInit() override;
  int OnExit() override;

  void OpenFile(const std::string& filename) const;

  EnvCreator& Creator();

  void AddEnv(std::unique_ptr<IEnvironment> env);
  [[nodiscard]] IEnvironment* GetEnv(const std::string& name);
  void DeleteEnv(const std::string& name);
  const EnvironmentList& EnvList() const;
  EnvironmentList& EnvList();

  [[nodiscard]] const std::string& ModelFileName() const {
    return model_filename_;
  }
  [[nodiscard]] const std::string& DbFileName() const {
    return db_filename_;
  }

  [[nodiscard]] detail::SqliteDatabase& Database() {
    return database_;
  }

  [[nodiscard]] const IModel& Model() const {
    return model_;
  }
  void GetTestBedList(NameIdMap& test_bed_list);

  bool AddSelectedChannel(const IItem& item);

  void RemoveSelectedChannel(int64_t item_index);

  [[nodiscard]] ItemList& SelectedList() {
    return selected_list_;
  }

  [[nodiscard]] util::log::TempDir& TemporaryDir() {
    return *temp_dir_;
  }

  [[nodiscard]] util::plot::GnuPlot& Plotter() {
    return plotter_;
  }

  IItem* GetSelectedItem(int64_t index);
  std::string FetchFile(int64_t index);

  void PlotSelectedItem(bool save_report);

 private:
  std::string notepad_; ///< Path to notepad.exe if it exist.
  EnvCreator creator_;  ///< Creator of environments
  EnvironmentList env_list_;
  ItemList selected_list_; ///< Item list with extra info

  std::string db_filename_;    ///< Path to the database SQLite file.
  std::string model_filename_; ///< Path to the model configuration file (XML).
  IModel model_;
  detail::SqliteDatabase database_;
  std::unique_ptr<util::log::TempDir> temp_dir_;
  util::plot::GnuPlot plotter_;

  void OnOpenLogFile(wxCommandEvent& event);
  void OnUpdateOpenLogFile(wxUpdateUIEvent& event);

  void OnModelFile(wxCommandEvent& event);

  void LoadEnvList();
  void SaveEnvList();

  bool InitSystem();
  bool CreateDb();
  bool InitDb();

  wxDECLARE_EVENT_TABLE();
};

wxDECLARE_APP(ReportExplorer);
} // end namespace
