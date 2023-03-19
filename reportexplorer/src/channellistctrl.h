/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
namespace ods::gui {

class ChannelListCtrl : public wxListView {
 public:
  explicit ChannelListCtrl(wxWindow* parent, bool selected_panel = false);
 protected:
  wxString OnGetItemText(long item, long column) const override;
 private:
  bool selected_panel_ = false;

};

}

