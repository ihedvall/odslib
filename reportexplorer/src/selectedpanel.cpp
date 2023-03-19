/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <unordered_set>
#include <util/logstream.h>
#include <ods/odsdef.h>

#include "selectedpanel.h"
#include "reportexplorerid.h"
#include "reportexplorer.h"
#include "mainframe.h"

using namespace util::log;

namespace ods::gui {
wxBEGIN_EVENT_TABLE(SelectedPanel, wxPanel) // NOLINT

wxEND_EVENT_TABLE()

SelectedPanel::SelectedPanel(wxWindow *parent)
    : wxPanel(parent) {
  list_ = new ChannelListCtrl(this, true);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(list_,1, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  SetSizerAndFit(main_sizer);
}

void SelectedPanel::RedrawSelectedList() {
  auto& app = wxGetApp();
  const auto& selected_list = app.SelectedList();
  list_->SetItemCount(static_cast<long>(selected_list.size()));
  list_->Refresh();
}

wxString SelectedPanel::OnGetItemText(long item, long column) const {
  auto& app = wxGetApp();
  const auto& selected_list = app.SelectedList();
  if ( item < 0 || item > selected_list.size()) {
    return wxEmptyString;
  }
  const auto& channel = selected_list[item];
  if (!channel) {
    return wxEmptyString;
  }

  switch (column) {
    case 0: // Name
      return wxString::FromUTF8(channel->Name());

    case 1:
      return wxString::FromUTF8(channel->Value<std::string>("UnitName"));

    case 2:
      return DataTypeToUserText(static_cast<DataType>(channel->BaseValue<int>("datatype")));

    case 3:
      return std::to_string(channel->Value<int64_t>("Samples"));

    case 4: {
      std::ostringstream temp;
      temp << channel->Value<std::string>("MeasName") << "/"
           << channel->Value<std::string>("FileName") << "/"
           << channel->Value<std::string>("TestName");
      return wxString::FromUTF8(temp.str());
    }

    default:
      break;
  }
  return wxEmptyString;
}

size_t SelectedPanel::GetSelectedCount() const {
  return list_ == nullptr ? 0 : static_cast<size_t>(list_->GetSelectedItemCount());
}

void SelectedPanel::RemoveSelected() {
  if (list_ == nullptr) {
    return;
  }
  auto& app = wxGetApp();
  auto& selected_list = app.SelectedList();

  std::unordered_set<int64_t> del_list;
  for (auto item = list_->GetFirstSelected(); item >= 0; item = list_->GetNextSelected(item)) {
    if (item < 0 || item >= selected_list.size()) {
      continue;
    }
    del_list.insert(selected_list[item]->ItemId());
  }
  std::erase_if(selected_list, [&] (const auto& chan) {
    return del_list.find(chan->ItemId()) != del_list.cend();
  });

  auto* main = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  if (main != nullptr) {
    main->RedrawSelectedList();
  }
}

void SelectedPanel::DoPlot(bool with_master) {
  if (list_ == nullptr) {
    return;
  }

  auto &app = wxGetApp();
  const auto &selected_list = app.SelectedList();
  for (auto& n_item:  selected_list) {
    if (n_item ) {
      n_item->SetAttribute({"Selected", 0});
    }
  }
  std::vector<IItem*> sel_list;
  for (auto sel_item = list_->GetFirstSelected(); sel_item >= 0; sel_item = list_->GetNextSelected(sel_item)) {
    if (sel_item < 0 || sel_item >= selected_list.size()) {
      continue;
    }
    auto* item = selected_list[sel_item].get();
    if (item != nullptr) {
      sel_list.push_back(item);
    }
  }
  if (sel_list.empty()) {
    LOG_ERROR() << "There is no selected items";
    return;
  }

  auto* y1_item = sel_list[0];
  const auto y1_unit = y1_item->BaseValue<int64_t>("unit");
  const auto y1_meas = y1_item->BaseValue<int64_t>("measurement");
   y1_item->SetAttribute({"Selected", "Y1"});

  // Select the Y1 and Y2 axis values
  for (size_t y_index = 1; y_index < sel_list.size(); ++y_index) {
    auto* y_item = sel_list[y_index];
    const auto y_unit = y_item->BaseValue<int64_t>("unit");
    y_item->SetAttribute({"Selected", y1_unit == y_unit ? "Y1" : "Y2"});
  }

  if (with_master) {
    for (auto& x_item:  selected_list) {
      if (x_item && x_item->Value<bool>("Independent") && x_item->BaseValue<int64_t>("measurement") == y1_meas) {
        x_item->SetAttribute({"Selected", "X"});
        break;
      }
    }
  }

  wxBusyCursor wait_cursor;
  app.PlotSelectedItem(false);
}

} // end namespace
