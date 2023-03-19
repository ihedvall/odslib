/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <unordered_set>
#include <util/stringutil.h>
#include <ods/odsdef.h>
#include <ods/databaseguard.h>
#include "channelpanel.h"
#include "reportexplorerid.h"
#include "reportexplorer.h"
#include "mainframe.h"
#include "selectedpanel.h"

namespace ods::gui {
wxBEGIN_EVENT_TABLE(ChannelPanel, wxPanel) // NOLINT
  EVT_UPDATE_UI(kIdAddChannel, ChannelPanel::OnSelected)
  EVT_BUTTON(kIdAddChannel, ChannelPanel::OnAdd)

  EVT_UPDATE_UI(kIdAddAllChannel, ChannelPanel::OnChannelExist)
  EVT_BUTTON(kIdAddAllChannel, ChannelPanel::OnAddAll)

  EVT_UPDATE_UI(kIdRemoveAllChannel, ChannelPanel::OnSelectedExist)
  EVT_BUTTON(kIdRemoveAllChannel, ChannelPanel::OnRemoveAll)

  EVT_UPDATE_UI(kIdRemoveChannel, ChannelPanel::OnSelectedCount)
  EVT_BUTTON(kIdRemoveChannel, ChannelPanel::OnRemove)
wxEND_EVENT_TABLE()

ChannelPanel::ChannelPanel(wxWindow *parent)
: wxPanel(parent),
  up_image_("UP", wxBITMAP_TYPE_BMP_RESOURCE),
  down_image_("DOWN", wxBITMAP_TYPE_BMP_RESOURCE),
  up_all_image_("UP_ALL", wxBITMAP_TYPE_BMP_RESOURCE),
  down_all_image_("DOWN_ALL", wxBITMAP_TYPE_BMP_RESOURCE) {
  list_ = new ChannelListCtrl(this, false);

  auto* down_button = new wxBitmapButton(this, kIdAddChannel, down_image_);
  auto* down_all_button = new wxBitmapButton(this, kIdAddAllChannel, down_all_image_);
  auto* up_button = new wxBitmapButton(this, kIdRemoveChannel, up_image_);
  auto* up_all_button = new wxBitmapButton(this, kIdRemoveAllChannel, up_all_image_);

  auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
  button_sizer->Add(down_button, 0, wxALIGN_TOP | wxALL, 5);
  button_sizer->Add(down_all_button, 0, wxALIGN_TOP | wxALL, 5);
  button_sizer->Add(up_button, 0, wxALIGN_TOP | wxALL, 5);
  button_sizer->Add(up_all_button, 0, wxALIGN_TOP | wxALL, 5);

  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(list_,1, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  main_sizer->Add(button_sizer, 0, wxALIGN_CENTER_HORIZONTAL| wxALL, 0);
  SetSizerAndFit(main_sizer);
}

void ChannelPanel::RedrawChannelList() {
  list_->SetItemCount(static_cast<long>(channel_list_.size()));
  list_->Refresh();
}

wxString ChannelPanel::OnGetItemText(long item, long column) const {
  if ( item < 0 || item > channel_list_.size()) {
    return wxEmptyString;
  }
  const auto& channel = channel_list_[item];
  if (!channel) {
    return wxEmptyString;
  }

  switch (column) {
    case 0: // Name
      return wxString::FromUTF8(channel->Name());

    case 1: {
      const auto index = channel->BaseValue<int64_t>("unit");
      const auto itr = unit_list_.find(index);
      if (itr != unit_list_.cend() ) {
        return wxString::FromUTF8(itr->second);
      }
      break;
    }

    case 2:
      return DataTypeToUserText(static_cast<DataType>(channel->BaseValue<int>("datatype")));

    case 3:
      return std::to_string(channel->Value<int64_t>("Samples"));

    case 4: {
      const auto meas_index = channel->BaseValue<int64_t>("measurement");
      const auto itr = meas_list_.find(meas_index);
      if (itr != meas_list_.cend() ) {
        return wxString::FromUTF8(itr->second);
      }
      break;
    }
    default:
      break;
  }
  return wxEmptyString;
}

void ChannelPanel::OnSelected(wxUpdateUIEvent &event) {
  const auto count = list_ != nullptr ? list_->GetSelectedItemCount() : 0;
  event.Enable(count > 0 && count < 1'000);
}

void ChannelPanel::OnChannelExist(wxUpdateUIEvent &event) {
  event.Enable(!channel_list_.empty() && channel_list_.size() < 1'000);
}

void ChannelPanel::OnSelectedExist(wxUpdateUIEvent &event) {
  auto& app = wxGetApp();
  const auto& selected_list = app.SelectedList();
  event.Enable(!selected_list.empty());
}

void ChannelPanel::OnSelectedCount(wxUpdateUIEvent &event) {
  auto* main = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  event.Enable(main != nullptr && main->GetSelectedCount() > 0);
}

void ChannelPanel::OnAddAll(wxCommandEvent &event) {
  if (list_ == nullptr) {
    return;
  }
  auto& app = wxGetApp();
  auto& database = app.Database();
  DatabaseGuard db_lock(database); // Speeding up database access
  for (const auto& channel : channel_list_) {
    if (!channel) {
      continue;
    }
    const auto add = app.AddSelectedChannel(*(channel));
    if (!add) {
      db_lock.Rollback();
      return;
    }
  }
  auto* main = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  if (main != nullptr) {
    main->RedrawSelectedList();
  }
}

void ChannelPanel::OnAdd(wxCommandEvent &event) {
  if (list_ == nullptr) {
    return;
  }
  auto& app = wxGetApp();
  auto& database = app.Database();
  DatabaseGuard db_lock(database); // Speeding up database access
  for (auto item = list_->GetFirstSelected(); item >= 0; item = list_->GetNextSelected(item)) {
    if (item < 0 || item >= channel_list_.size()) {
      continue;
    }
    const auto add = app.AddSelectedChannel(*(channel_list_[item]));
    if (!add) {
      db_lock.Rollback();
      return;
    }
  }
  auto* main = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  if (main != nullptr) {
    main->RedrawSelectedList();
  }
}

void ChannelPanel::OnRemoveAll(wxCommandEvent &event) {
  auto& app = wxGetApp();
  auto& selected_list = app.SelectedList();
  selected_list.clear();

  auto* main = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  if (main != nullptr) {
    main->RedrawSelectedList();
  }
}

void ChannelPanel::OnRemove(wxCommandEvent &event) {
  auto* main = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  if (main != nullptr) {
    main->RemoveSelected();
  }
}

} // end namespace