/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <wx/treelist.h>
#include <wx/listctrl.h>
#include <wx/checkbox.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/srchctrl.h>
#include <ods/odsdef.h>
#include <ods/iitem.h>
#include "meastreedata.h"
#include "measselectionpanel.h"
#include "channelpanel.h"
#include "selectedpanel.h"
namespace ods::gui {

class MeasurementTab : public wxPanel {
 public:
  explicit MeasurementTab(wxWindow* parent);
  ~MeasurementTab() override = default;

  void Update() override;

  void FetchFileFromDb(int64_t parent);
  void FetchMeasFromDb(int64_t parent);

  void SelectedMeas(const MeasTreeData& data);
  [[nodiscard]] const MeasTreeData& SelectedMeas() const;
  void RedrawSelectedList();
  [[nodiscard]] size_t GetSelectedCount() const;
  void RemoveSelected();

 private:
  MeasSelectionPanel* selection_ = nullptr;
  ChannelPanel* channel_ = nullptr;
  SelectedPanel* selected_ = nullptr;

  wxCheckBox* last_test_ = nullptr;
  wxComboBox* test_bed_ = nullptr;
  wxDatePickerCtrl* from_date_ = nullptr;
  wxDatePickerCtrl* to_date_ = nullptr;
  wxSearchCtrl* test_name_ = nullptr;
  wxSearchCtrl* file_name_ = nullptr;
  wxSearchCtrl* meas_name_ = nullptr;

  NameIdMap test_bed_filter_list_;
  MeasTreeData selected_meas_;


  [[nodiscard]] bool IsLastTest() const;
  [[nodiscard]] int64_t TestBedIndex() const;
  [[nodiscard]] uint64_t FromDate() const;
  [[nodiscard]] uint64_t ToDate() const;
  [[nodiscard]] std::string TestName() const;
  [[nodiscard]] std::string FileName() const;
  [[nodiscard]] std::string MeasName() const;

  void FetchFilterFromDb();
  void FetchTestFromDb();
  void FetchChannelFromDb();

  void RedrawTestList();
  void RedrawFileList();
  void RedrawMeasList();


  void OnLastTest(wxCommandEvent& event);

  void OnTestBedChanged(wxCommandEvent& event);
  void OnDateChanged(wxDateEvent& event);
  void OnTestName(wxCommandEvent& event);
  void OnFileName(wxCommandEvent& event);
  void OnMeasName(wxCommandEvent& event);
  void OnContextSelected(wxListEvent& event);
  void OnListNotEmpty(wxUpdateUIEvent& event);
  void OnListHaveMaster(wxUpdateUIEvent& event);
  void OnPlotWithSample(wxCommandEvent& event);
  void OnPlotWithMaster(wxCommandEvent& event);
  wxDECLARE_EVENT_TABLE();
};



} //end namespace
