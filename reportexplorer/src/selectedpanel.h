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

class SelectedPanel : public wxPanel {
 public:
  explicit SelectedPanel(wxWindow *parent);
  void RedrawSelectedList();
  [[nodiscard]] wxString OnGetItemText(long item, long column) const;
  [[nodiscard]] size_t GetSelectedCount() const;
  void RemoveSelected();
  void DoPlot(bool with_master);
 private:
  ChannelListCtrl *list_ = nullptr;

 wxDECLARE_EVENT_TABLE();
};

} // end namespace