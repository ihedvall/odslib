/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */


#include "channellistctrl.h"
#include "reportexplorerid.h"
#include "channelpanel.h"
#include "selectedpanel.h"
namespace ods::gui {

ChannelListCtrl::ChannelListCtrl(wxWindow *parent, bool selected_panel )
: wxListView(parent, selected_panel ? kIdSelectedList : kIdChannelList, wxDefaultPosition,
             {900, 100}, wxLC_REPORT | wxLC_VIRTUAL),
  selected_panel_(selected_panel) {
  AppendColumn("Channel", wxLIST_FORMAT_LEFT, 200 );
  AppendColumn("Unit", wxLIST_FORMAT_LEFT, 75 );
  AppendColumn("Type", wxLIST_FORMAT_LEFT, 75 );
  AppendColumn("Samples", wxLIST_FORMAT_LEFT, 75);
  AppendColumn("Measurement", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER );
}

wxString ChannelListCtrl::OnGetItemText(long item, long column) const {
  if (selected_panel_) {
    auto* panel = dynamic_cast<const SelectedPanel*>(GetParent());
    return panel != nullptr ? panel->OnGetItemText(item, column) : wxListCtrlBase::OnGetItemText(item, column);
  } else {
    auto* panel = dynamic_cast<const ChannelPanel*>(GetParent());
    return panel != nullptr ? panel->OnGetItemText(item, column) : wxListCtrlBase::OnGetItemText(item, column);
  }
}

} // end namespace