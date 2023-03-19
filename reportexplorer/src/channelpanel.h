/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/wx.h>
#include <ods/iitem.h>
#include <ods/odsdef.h>
#include "channellistctrl.h"

namespace ods::gui {

class ChannelPanel : public wxPanel {
 public:
  explicit ChannelPanel(wxWindow *parent);

  [[nodiscard]] IdNameMap& MeasList() {
    return meas_list_;
  }
  [[nodiscard]] IdNameMap& UnitList() {
    return unit_list_;
  }
  [[nodiscard]] ItemList& ChannelList() {
    return channel_list_;
  }
  void RedrawChannelList();
  wxString OnGetItemText(long item, long column) const;
 private:
  ChannelListCtrl *list_ = nullptr;
  IdNameMap meas_list_;
  ItemList  channel_list_;
  IdNameMap unit_list_;
  wxBitmap up_image_;
  wxBitmap down_image_;
  wxBitmap up_all_image_;
  wxBitmap down_all_image_;

  void OnSelected(wxUpdateUIEvent &event);
  void OnChannelExist(wxUpdateUIEvent &event);
  void OnSelectedExist(wxUpdateUIEvent &event);
  void OnSelectedCount(wxUpdateUIEvent &event);
  void OnAddAll(wxCommandEvent& event);
  void OnAdd(wxCommandEvent& event);
  void OnRemoveAll(wxCommandEvent& event);
  void OnRemove(wxCommandEvent& event);
 wxDECLARE_EVENT_TABLE();
};

} // end namespace
