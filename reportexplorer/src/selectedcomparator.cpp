/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "selectedcomparator.h"
namespace ods::gui {

int SelectedComparator::Compare(wxTreeListCtrl *treelist,
                                unsigned int column,
                                wxTreeListItem first,
                                wxTreeListItem second) {
  if (treelist == nullptr) {
    return -1;
  }

  switch (column) {

    case 0: {// Selection + Name
      const auto checked1 = treelist->GetCheckedState(first) == wxCHK_CHECKED;
      const auto checked2 = treelist->GetCheckedState(second) == wxCHK_CHECKED;
      if (checked1 && !checked2) {
        return -1;
      } else if (!checked1 && checked2) {
        return 1;
      }
      break;
    }

    case 4:  {
      const auto nof1 = std::stoull(treelist->GetItemText(first, column).ToStdString());
      const auto nof2 = std::stoull(treelist->GetItemText(second, column).ToStdString());
      if (nof1 > nof2) {
        return 1;
      } else if (nof1 < nof2) {
        return -1;
      }
      break;
    }

    default:  {
      const auto text1 = treelist->GetItemText(first, column).utf8_string();
      const auto text2 = treelist->GetItemText(second, column).utf8_string();
      const auto cmp =  stricmp(text1.c_str(),text2.c_str());
      if (cmp != 0) {
        return cmp;
      }
      break;
    }
  }
  const auto name1 = treelist->GetItemText(first).utf8_string();
  const auto name2 = treelist->GetItemText(second).utf8_string();
  return stricmp(name1.c_str(),name2.c_str());
}
} // end namespace