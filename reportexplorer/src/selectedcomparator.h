/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <wx/treelist.h>
namespace ods::gui {

class SelectedComparator : public wxTreeListItemComparator {
 public:
  int Compare(wxTreeListCtrl *treelist, unsigned column, wxTreeListItem first, wxTreeListItem second) override;

};

} // end namespace