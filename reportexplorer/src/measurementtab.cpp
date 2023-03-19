/*
* Copyright 2022 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <ctime>
#include <unordered_set>
#include <wx/splitter.h>
#include <util/logstream.h>
#include <util/timestamp.h>
#include <ods/databaseguard.h>
#include "measurementtab.h"
#include "reportexplorerid.h"
#include "reportexplorer.h"

using namespace util::log;
using namespace util::time;


namespace {
  constexpr std::string_view kMaxTestName = "XXXX_YYYY-MM-DD_XXX";
  constexpr std::string_view kMaxFileName = "XXXXXXXXXXXXXX.mf4";

  void RedrawComboBox(wxComboBox& box, ods::NameIdMap& list) {
    const auto selection = box.GetValue().ToStdString();
    int selected = -1;
    box.Clear();
    box.SetEditable(true);
    box.Append("*");
    if (selection == "*") {
      selected = 0;
    }
    for (const auto& itr : list) {
      const auto index = box.Append(itr.first);
      if (itr.first == selection) {
        selected = index;
      }
    }
    if (selected >= 0) {
      box.SetSelection(selected);
    }
    box.SetValue(selection);
  }
}

namespace ods::gui {

wxBEGIN_EVENT_TABLE(MeasurementTab, wxPanel) // NOLINT
  EVT_CHECKBOX(kIdLastTestFilter, MeasurementTab::OnLastTest)
  EVT_COMBOBOX(kIdTestBedFilter, MeasurementTab::OnTestBedChanged)
  EVT_DATE_CHANGED(kIdFromFilter, MeasurementTab::OnDateChanged)
  EVT_DATE_CHANGED(kIdToFilter, MeasurementTab::OnDateChanged)

  EVT_SEARCHCTRL_SEARCH_BTN(kIdTestNameFilter, MeasurementTab::OnTestName)
  EVT_SEARCHCTRL_CANCEL_BTN(kIdTestNameFilter, MeasurementTab::OnTestName)
  EVT_TEXT_ENTER(kIdTestNameFilter, MeasurementTab::OnTestName)

  EVT_SEARCHCTRL_SEARCH_BTN(kIdFileNameFilter, MeasurementTab::OnFileName)
  EVT_SEARCHCTRL_CANCEL_BTN(kIdFileNameFilter, MeasurementTab::OnFileName)
  EVT_TEXT_ENTER(kIdFileNameFilter, MeasurementTab::OnFileName)

  EVT_SEARCHCTRL_SEARCH_BTN(kIdMeasNameFilter, MeasurementTab::OnMeasName)
  EVT_SEARCHCTRL_CANCEL_BTN(kIdMeasNameFilter, MeasurementTab::OnMeasName)
  EVT_TEXT_ENTER(kIdMeasNameFilter, MeasurementTab::OnMeasName)

  EVT_LIST_ITEM_RIGHT_CLICK(kIdSelectedList, MeasurementTab::OnContextSelected)
  EVT_UPDATE_UI(kIdPlotWithSample, MeasurementTab::OnListNotEmpty)
  EVT_UPDATE_UI(kIdPlotWithMaster, MeasurementTab::OnListHaveMaster)
  EVT_MENU(kIdPlotWithSample, MeasurementTab::OnPlotWithSample)
  EVT_MENU(kIdPlotWithMaster, MeasurementTab::OnPlotWithMaster)
wxEND_EVENT_TABLE()

MeasurementTab::MeasurementTab(wxWindow *parent)
    : wxPanel(parent) {

  FetchFilterFromDb();

  auto *main_splitter = new wxSplitterWindow(this);
  auto *right_splitter = new wxSplitterWindow(main_splitter);

  selection_ = new MeasSelectionPanel(main_splitter);
  channel_ = new ChannelPanel(right_splitter);
  selected_ = new SelectedPanel(right_splitter);

  right_splitter->SplitHorizontally(channel_, selected_, 400);
  main_splitter->SplitVertically(selection_, right_splitter, 350);

  // LAST TEST
  last_test_ = new wxCheckBox(this, kIdLastTestFilter, "");
  auto* last_test_label = new wxStaticText(this, wxID_ANY, L"Last");
  auto* last_test_sizer = new wxBoxSizer(wxVERTICAL);
  last_test_sizer->Add(last_test_label, 0, wxALIGN_LEFT | wxALL, 1);
  last_test_sizer->Add(last_test_, 0, wxALIGN_LEFT | wxALL, 5);

  // TEST BED FILTER
  test_bed_ = new wxComboBox(this, kIdTestBedFilter, "*", wxDefaultPosition, wxDefaultSize,
                             0, nullptr,wxCB_DROPDOWN | wxTE_PROCESS_ENTER);
  RedrawComboBox(*test_bed_, test_bed_filter_list_);
  auto* test_bed_label = new wxStaticText(this, wxID_ANY, L"Test Bed");
  auto* test_bed_sizer = new wxBoxSizer(wxVERTICAL);
  test_bed_sizer->Add(test_bed_label, 0, wxALIGN_LEFT | wxALL, 1);
  test_bed_sizer->Add(test_bed_, 0, wxALIGN_LEFT | wxALL, 1);

  // FROM DATE FILTER
  wxDateTime from(static_cast<time_t>(0));
  from.ResetTime();
  from_date_ = new wxDatePickerCtrl(this, kIdFromFilter, from);
  auto* from_label = new wxStaticText(this, wxID_ANY, L"From");
  auto* from_sizer = new wxBoxSizer(wxVERTICAL);
  from_sizer->Add(from_label, 0, wxALIGN_LEFT | wxALL, 1);
  from_sizer->Add(from_date_, 0, wxALIGN_LEFT | wxALL, 1);

  //  TO DATE FILTER
  wxDateTime now(time(nullptr));
  now.ResetTime();
  to_date_ = new wxDatePickerCtrl(this, kIdToFilter, now);
  auto* to_label = new wxStaticText(this, wxID_ANY, L"To");
  auto* to_sizer = new wxBoxSizer(wxVERTICAL);
  to_sizer->Add(to_label, 0, wxALIGN_LEFT | wxALL, 1);
  to_sizer->Add(to_date_, 0, wxALIGN_LEFT | wxALL, 1);

  // TEST NAME FILTER
  test_name_ = new wxSearchCtrl(this, kIdTestNameFilter, kMaxTestName.data(), wxDefaultPosition, wxDefaultSize,
                                wxTE_PROCESS_ENTER | wxTE_NOHIDESEL);
  test_name_->ShowCancelButton(true);
  test_name_->ShowSearchButton(true);
  test_name_->SetValue(L"*");
  auto* test_label = new wxStaticText(this, wxID_ANY, L"Name");
  auto* test_sizer = new wxBoxSizer(wxVERTICAL);
  test_sizer->Add(test_label, 0, wxALIGN_LEFT | wxALL, 1);
  test_sizer->Add(test_name_, 0, wxALIGN_LEFT | wxALL, 1);

  // FILE NAME FILTER
  file_name_ = new wxSearchCtrl(this, kIdFileNameFilter, kMaxFileName.data(), wxDefaultPosition, wxDefaultSize,
                                wxTE_PROCESS_ENTER | wxTE_NOHIDESEL);
  file_name_->ShowCancelButton(true);
  file_name_->ShowSearchButton(true);
  file_name_->SetValue(L"*");
  auto* file_label = new wxStaticText(this, wxID_ANY, L"Name");
  auto* file_sizer = new wxBoxSizer(wxVERTICAL);
  file_sizer->Add(file_label, 0, wxALIGN_LEFT | wxALL, 1);
  file_sizer->Add(file_name_, 0, wxALIGN_LEFT | wxALL, 1);

  // MEAS NAME FILTER
  meas_name_ = new wxSearchCtrl(this, kIdMeasNameFilter, kMaxFileName.data(), wxDefaultPosition, wxDefaultSize,
                                  wxTE_PROCESS_ENTER | wxTE_NOHIDESEL);
  meas_name_->ShowCancelButton(true);
  meas_name_->ShowSearchButton(true);
  meas_name_->SetValue(L"*");
  auto* meas_label = new wxStaticText(this, wxID_ANY, L"Name");
  auto* meas_sizer = new wxBoxSizer(wxVERTICAL);
  meas_sizer->Add(meas_label, 0, wxALIGN_LEFT | wxALL, 1);
  meas_sizer->Add(meas_name_, 0, wxALIGN_LEFT | wxALL, 1);

  auto* test_filter_box = new wxStaticBoxSizer(wxHORIZONTAL,this, L"Test Filter");
  test_filter_box->Add(test_bed_sizer, 0, wxALL, 1);
  test_filter_box->Add(last_test_sizer, 0, wxALL, 1);
  test_filter_box->Add(from_sizer, 0, wxALL, 1);
  test_filter_box->Add(to_sizer, 0, wxALL, 1);
  test_filter_box->Add(test_sizer, 0, wxALL, 1);

  auto* file_filter_box = new wxStaticBoxSizer(wxHORIZONTAL,this, L"File Filter");
  file_filter_box->Add(file_sizer, 0, wxALIGN_LEFT |wxALL, 1);

  auto* meas_filter_box = new wxStaticBoxSizer(wxHORIZONTAL,this, L"Measurement Filter");
  meas_filter_box->Add(meas_sizer, 0, wxALIGN_LEFT |wxALL, 1);

  auto* filter_box = new wxBoxSizer(wxHORIZONTAL);
  filter_box->Add(test_filter_box,0, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  filter_box->Add(file_filter_box,0, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  filter_box->Add(meas_filter_box,0, wxALIGN_LEFT | wxALL | wxEXPAND, 0);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(main_splitter,1, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  main_sizer->Add(filter_box,0, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  SetSizerAndFit(main_sizer);
}

bool MeasurementTab::IsLastTest() const {
  return last_test_ != nullptr && last_test_->IsChecked();
}

int64_t MeasurementTab::TestBedIndex() const {
  int64_t index = 0;
  if (test_bed_ != nullptr) {
    const std::string sel = test_bed_->GetValue().ToStdString();
    const auto itr = test_bed_filter_list_.find(sel);
    if (itr != test_bed_filter_list_.cend()) {
      index = itr->second;
    }
  }
  return index;
}

uint64_t MeasurementTab::FromDate() const {
  uint64_t time = 0;
  if (from_date_ != nullptr) {
    const auto date = from_date_->GetValue();
    auto sec1970 = date.GetTicks();
    if (sec1970 < 0) {
      sec1970 = 0;
    }
    time = sec1970;
    time*= 1'000'000'000;
  }
  return time;
}

uint64_t MeasurementTab::ToDate() const {
  uint64_t time = TimeStampToNs();
  if (to_date_ != nullptr) {
    const auto date = to_date_->GetValue();
    time = date.GetTicks();
    time*= 1'000'000'000;
  }
  return time;
}

std::string MeasurementTab::TestName() const {
  std::string temp;
  if (test_name_ != nullptr) {
    temp = test_name_->GetValue().ToStdString();
  }
  return WildcardToSql(temp);
}

std::string MeasurementTab::FileName() const {
  std::string temp;
  if (file_name_ != nullptr) {
    temp = file_name_->GetValue().ToStdString();
  }
  return WildcardToSql(temp);
}

std::string MeasurementTab::MeasName() const {
  std::string temp;
  if (meas_name_ != nullptr) {
    temp = meas_name_->GetValue().ToStdString();
  }
  return WildcardToSql(temp);
}
void MeasurementTab::FetchFilterFromDb() {
  test_bed_filter_list_.clear();
  auto& app = wxGetApp();
  app.GetTestBedList(test_bed_filter_list_);

}

void MeasurementTab::FetchTestFromDb() {
  auto& app = wxGetApp();
  auto& database = app.Database();
  auto& model = app.Model();

  // SETUP TEST FILTER
  const auto* test_table = model.GetBaseId(BaseId::AoTest);
  if (test_table == nullptr || selection_ == nullptr) {
    return;
  }
  const auto* test_name_column = test_table->GetColumnByBaseName("name");
  const auto* test_bed_column = test_table->GetColumnByName("TestBed");
  const auto* created_column = test_table->GetColumnByBaseName("ao_created");

  if (test_name_column == nullptr || test_bed_column == nullptr || created_column == nullptr) {
    return;
  }

  SqlFilter test_filter;
  const auto test_bed = TestBedIndex();
  if (test_bed > 0 ) {
    test_filter.AddWhere(*test_bed_column, SqlCondition::Equal, test_bed);
  }
  if (IsLastTest()) {
    test_filter.AddOrder(*test_name_column, SqlCondition::OrderByDesc);
    test_filter.AddLimit(SqlCondition::LimitNofRows, 1);
  } else {
    test_filter.AddOrder(*test_name_column, SqlCondition::OrderByDesc);
    test_filter.AddWhere(*created_column, SqlCondition::GreaterEQ, FromDate());
    test_filter.AddWhere(*created_column, SqlCondition::LessEQ, ToDate());
    test_filter.AddWhere(*test_name_column, SqlCondition::Like, TestName());
  }

  auto& test_list = selection_->TestList();
  test_list.clear();

  DatabaseGuard db_lock(database);
  try {
    database.FetchItemList(*test_table, test_list, test_filter);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch the test list. Error: " << err.what();
    db_lock.Rollback();
    return;
  }
}

void MeasurementTab::FetchFileFromDb(int64_t parent) {
  auto& app = wxGetApp();
  auto& database = app.Database();
  auto& model = app.Model();

  // SETUP TEST FILTER
  const auto* table = model.GetBaseId(BaseId::AoSubTest);
  if (table == nullptr || parent <= 0 || selection_ == nullptr) {
    return;
  }
  auto& file_list = selection_->FileList();
  file_list.clear();
  const auto* name_column = table->GetColumnByBaseName("name");
  const auto* parent_column = table->GetColumnByBaseName("parent_test");
  if (name_column == nullptr || parent_column == nullptr) {
    return;
  }

  SqlFilter filter;
  filter.AddWhere(*parent_column, SqlCondition::Equal, parent);
  filter.AddWhere(*name_column, SqlCondition::Like, FileName());
  filter.AddOrder(*name_column, SqlCondition::OrderByAsc);

  DatabaseGuard db_lock(database);
  try {
    database.FetchItemList(*table, file_list, filter);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch the file list. Error: " << err.what();
    db_lock.Rollback();
    return;
  }
}

void MeasurementTab::FetchMeasFromDb(int64_t parent) {
  auto& app = wxGetApp();
  auto& database = app.Database();
  auto& model = app.Model();

  // SETUP TEST FILTER
  const auto* table = model.GetBaseId(BaseId::AoMeasurement);
  if (table == nullptr || parent <= 0) {
    return;
  }

  const auto* name_column = table->GetColumnByBaseName("name");
  const auto* parent_column = table->GetColumnByBaseName("test");
  const auto* order_column = table->GetColumnByName("MeasIndex");
  if (name_column == nullptr || parent_column == nullptr || selection_ == nullptr) {
    return;
  }
  auto& meas_list = selection_->MeasList();
  meas_list.clear();
  SqlFilter filter;
  filter.AddWhere(*parent_column, SqlCondition::Equal, parent);
  filter.AddWhere(*name_column, SqlCondition::Like, MeasName());
  if (order_column != nullptr) {
    filter.AddOrder(*order_column, SqlCondition::OrderByAsc);
  }

  DatabaseGuard db_lock(database);
  try {
    database.FetchItemList(*table, meas_list, filter);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch the file list. Error: " << err.what();
    db_lock.Rollback();
    return;
  }
}

void MeasurementTab::OnLastTest(wxCommandEvent& event) {
  if (event.IsChecked() ) {
    from_date_->Enable(false);
    to_date_->Enable(false);
    test_name_->Enable(false);
  } else {
    from_date_->Enable(true);
    to_date_->Enable(true);
    test_name_->Enable(true);
  }
  RedrawTestList();
}

void MeasurementTab::RedrawTestList() {
  if (selection_ != nullptr) {
    FetchTestFromDb();
    selection_->RedrawTestList();
  }
}

void MeasurementTab::RedrawFileList() {
  if (selection_ != nullptr) {
    selection_->RedrawFileList();
  }
}

void MeasurementTab::RedrawMeasList() {
  if (selection_ != nullptr) {
    selection_->RedrawMeasList();
  }
}

void MeasurementTab::Update() {
  wxWindow::Update();
  RedrawTestList();
}

void MeasurementTab::OnTestBedChanged(wxCommandEvent &event) {
   RedrawTestList();
}

void MeasurementTab::OnDateChanged(wxDateEvent& event) {
  RedrawTestList();
}

void MeasurementTab::OnTestName(wxCommandEvent &event) {
  RedrawTestList();
}

void MeasurementTab::OnFileName(wxCommandEvent &event) {
  RedrawFileList();
}

void MeasurementTab::OnMeasName(wxCommandEvent &event) {
  RedrawMeasList();
}

void MeasurementTab::SelectedMeas(const MeasTreeData &data) {
  selected_meas_ = data;
  FetchChannelFromDb();
  if (channel_ != nullptr) {
    channel_->RedrawChannelList();
  }
}

void MeasurementTab::FetchChannelFromDb() {
  auto& app = wxGetApp();
  auto& database = app.Database();
  auto& model = app.Model();

  const auto* file_table = model.GetBaseId(BaseId::AoSubTest);
  const auto* meas_table = model.GetBaseId(BaseId::AoMeasurement);
  const auto* channel_table = model.GetBaseId(BaseId::AoMeasurementQuantity);
  const auto* unit_table = model.GetBaseId(BaseId::AoUnit);
  if (file_table == nullptr || meas_table == nullptr ||
      channel_table == nullptr || unit_table == nullptr ) {
    return;
  }

  const auto* file_name_column = file_table->GetColumnByBaseName("name");
  const auto* file_parent_column = file_table->GetColumnByBaseName("parent_test");
  const auto* meas_name_column = meas_table->GetColumnByBaseName("name");
  const auto* meas_parent_column = meas_table->GetColumnByBaseName("test");
  const auto* meas_id_column = meas_table->GetColumnByBaseName("id");
  const auto* channel_name_column = channel_table->GetColumnByBaseName("name");
  const auto* channel_parent_column = channel_table->GetColumnByBaseName("measurement");

  if (file_name_column == nullptr || file_parent_column == nullptr ||
      meas_name_column == nullptr || meas_parent_column == nullptr ||
      meas_id_column == nullptr ||
      channel_parent_column == nullptr || channel_name_column == nullptr ) {
    return;
  }

  SqlFilter file_filter;
  SqlFilter meas_filter;
  SqlFilter channel_filter;
  IdNameMap file_list;
  auto& meas_list = channel_->MeasList();
  auto& channel_list = channel_->ChannelList();
  auto& unit_list = channel_->UnitList();
  meas_list.clear();
  channel_list.clear();
  unit_list.clear();

  DatabaseGuard db_lock(database);
  try {

    switch (selected_meas_.Type()) {
      case MeasTreeItem::TestItem: {
        if (selected_meas_.Index() > 0) {
          file_filter.AddWhere(*file_parent_column, SqlCondition::Equal, selected_meas_.Index());
          file_filter.AddWhere(*file_name_column, SqlCondition::Like, FileName());
          database.FetchNameMap(*file_table, file_list, file_filter);
        }
        if (!file_list.empty()) {
          meas_filter.AddWhere(*meas_parent_column, SqlCondition::In, file_list);
          meas_filter.AddWhere(*meas_name_column, SqlCondition::Like, MeasName());
          database.FetchNameMap(*meas_table, meas_list, meas_filter);
        }
        break;
      }

      case MeasTreeItem::FileItem: {
        if (selected_meas_.Index() > 0) {
          meas_filter.AddWhere(*meas_parent_column, SqlCondition::Equal, selected_meas_.Index());
          meas_filter.AddWhere(*meas_name_column, SqlCondition::Like, MeasName());
          database.FetchNameMap(*meas_table, meas_list, meas_filter);
        }
        break;
      }

      case MeasTreeItem::MeasItem: {
        if (selected_meas_.Index() > 0) {
          meas_filter.AddWhere(*meas_id_column, SqlCondition::Equal, selected_meas_.Index());
          database.FetchNameMap(*meas_table, meas_list, meas_filter);
        }
        break;
      }

      default:
        break;
    }
    if (!meas_list.empty()) {
      channel_filter.AddWhere(*channel_parent_column, SqlCondition::In, meas_list);
      channel_filter.AddOrder(*channel_name_column, SqlCondition::OrderByAsc);
      database.FetchItemList(*channel_table, channel_list, channel_filter);
    }
    SqlFilter no_filter;
    database.FetchNameMap(*unit_table, unit_list, no_filter);
   } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fetch the channel list. Error: " << err.what();
    db_lock.Rollback();
    return;
  }

}

const MeasTreeData &MeasurementTab::SelectedMeas() const {
  return selected_meas_;
}

void MeasurementTab::RedrawSelectedList() {
  if (selected_ != nullptr) {
    selected_->RedrawSelectedList();
  }
}

size_t MeasurementTab::GetSelectedCount() const {
  return selected_ == nullptr ? 0 : selected_->GetSelectedCount();
}

void MeasurementTab::RemoveSelected() {
  if (selected_ != nullptr) {
    selected_->RemoveSelected();
  }
}

void MeasurementTab::OnContextSelected(wxListEvent &event) {
  wxMenu menu;
  menu.Append(kIdPlotWithSample, L"Plot against Sample" );
  menu.Append(kIdPlotWithMaster, L"Plot against Master" );
  PopupMenu(&menu);
}

void MeasurementTab::OnListNotEmpty(wxUpdateUIEvent &event) {
  auto &app = wxGetApp();
  const auto &selected_list = app.SelectedList();
  event.Enable(!selected_list.empty());
}

void MeasurementTab::OnListHaveMaster(wxUpdateUIEvent &event) {
  auto &app = wxGetApp();
  const auto &selected_list = app.SelectedList();
  const auto have_master = std::ranges::any_of(selected_list, [] (const auto& itr) {
    return itr && itr->template Value<bool>("Independent");
  });
  event.Enable(have_master);
}

void MeasurementTab::OnPlotWithSample(wxCommandEvent &event) {
  if (selected_ != nullptr) {
    selected_->DoPlot(false);
  }
}

void MeasurementTab::OnPlotWithMaster(wxCommandEvent &event) {
  if (selected_ != nullptr) {
    selected_->DoPlot(true);
  }
}

}