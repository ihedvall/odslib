/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <unordered_set>
#include <util/timestamp.h>
#include "measselectionpanel.h"
#include "reportexplorerid.h"
#include "meastreedata.h"
#include "measurementtab.h"

using namespace util::time;

namespace {

constexpr int kTestCloseBmp = 0;
constexpr int kTestOpenBmp = 1;
constexpr int kFileCloseBmp = 2;
constexpr int kFileOpenBmp = 3;
constexpr int kMeasCloseBmp = 4;
constexpr int kMeasOpenBmp = 5;

}

namespace ods::gui {

wxBEGIN_EVENT_TABLE(MeasSelectionPanel, wxPanel) // NOLINT
    EVT_TREELIST_ITEM_EXPANDING(kIdSelectionList, MeasSelectionPanel::OnLeftExpanding)
    EVT_TREELIST_SELECTION_CHANGED(kIdSelectionList, MeasSelectionPanel::OnSelectionChange)
wxEND_EVENT_TABLE()

MeasSelectionPanel::MeasSelectionPanel(wxWindow *parent) :
    wxPanel(parent),
    image_list_(16, 16, false, 6) {
  image_list_.Add(wxBitmap("TREE_LIST", wxBITMAP_TYPE_BMP_RESOURCE));
  left_ = new wxTreeListCtrl(this, kIdSelectionList, wxDefaultPosition, {300, 800},
                             wxTL_SINGLE);
  left_->SetImageList(&image_list_);

  left_->AppendColumn(L"Name");
  left_->AppendColumn(L"Created");

  auto *main_sizer = new wxBoxSizer(wxVERTICAL);
  main_sizer->Add(left_, 1, wxALIGN_LEFT | wxALL | wxEXPAND, 0);
  SetSizerAndFit(main_sizer);
}

void MeasSelectionPanel::RedrawTestList() {
  // Delete all tests which are not in the test_list_.
  // First make a list of test indexes.
  std::unordered_set<int64_t> new_list; // All tests
  std::unordered_set<int64_t> curr_list; // Old tests
  for (const auto &new_test: test_list_) {
    new_list.insert(new_test->ItemId());
  }
  std::vector<wxTreeListItem> del_list;
  auto root_item = left_->GetRootItem();
  for (auto item = left_->GetFirstChild(root_item); item.IsOk(); item = left_->GetNextSibling(item)) {
    auto *data = dynamic_cast<MeasTreeData *>(left_->GetItemData(item));
    if (data == nullptr || new_list.find(data->Index()) == new_list.cend()) {
      del_list.emplace_back(item);
    } else {
      curr_list.insert(data->Index());
    }
  }
  for (const auto &del: del_list) {
    left_->DeleteItem(del);
  }

  for (const auto &test: test_list_) {
    if (curr_list.find(test->ItemId()) == curr_list.cend()) {
      auto test_item = left_->AppendItem(root_item, test->Name(), kTestCloseBmp, kTestOpenBmp,
                                         new MeasTreeData(MeasTreeItem::TestItem, 0, test->ItemId()));
      const auto ns1970 = IsoTimeToNs(test->BaseValue<std::string>("ao_created"));
      left_->SetItemText(test_item, 1, NsToLocalDate(ns1970));
      left_->AppendItem(test_item, "Needs Update", kFileCloseBmp, kFileOpenBmp,
                        new MeasTreeData(MeasTreeItem::FileItem, test->ItemId(), 0));
    }
  }

}

void MeasSelectionPanel::RedrawFileList() {
  auto root_item = left_->GetRootItem();
  for (auto test_item = left_->GetFirstChild(root_item);
       test_item.IsOk();
       test_item = left_->GetNextSibling(test_item)) {
    auto *test_data = dynamic_cast<MeasTreeData *>(left_->GetItemData(test_item));
    auto child_item = left_->GetFirstChild(test_item);
    if (test_data != nullptr && !child_item.IsOk()) {
      left_->AppendItem(test_item, "Needs Update", kFileCloseBmp, kFileOpenBmp,
                        new MeasTreeData(MeasTreeItem::MeasItem, test_data->Index(), 0));
      continue;
    }
    auto *child_data = child_item.IsOk() ? dynamic_cast<MeasTreeData *>(left_->GetItemData(child_item)) : nullptr;
    const bool expanded = left_->IsExpanded(test_item);
    if (test_data == nullptr) {
      continue;
    }
    if (child_data != nullptr && child_data->Index() > 0) {
      RedrawFileList(test_item, test_data->Index());
      if (expanded) {
        left_->Expand(test_item);
      } else {
        left_->Collapse(test_item);
      }
    }
  }

}
void MeasSelectionPanel::RedrawMeasList() {

  auto root_item = left_->GetRootItem();
  for (auto test_item = left_->GetFirstChild(root_item);
       test_item.IsOk();
       test_item = left_->GetNextSibling(test_item)) {
    auto *test_data = dynamic_cast<MeasTreeData *>(left_->GetItemData(test_item));
    auto child_item = left_->GetFirstChild(test_item);
    auto *child_data = child_item.IsOk() ? dynamic_cast<MeasTreeData *>(left_->GetItemData(child_item)) : nullptr;
    if (test_data == nullptr || !child_item.IsOk() || child_data == nullptr || child_data->Index() <= 0) {
      continue;
    }
    for (auto file_item = left_->GetFirstChild(test_item);
         file_item.IsOk();
         file_item = left_->GetNextSibling(file_item)) {
      auto *file_data = dynamic_cast<MeasTreeData *>(left_->GetItemData(file_item));
      auto meas_item = left_->GetFirstChild(file_item);
      if (file_data != nullptr && !meas_item.IsOk()) {
        left_->AppendItem(file_item, "Needs Update", kMeasCloseBmp, kMeasOpenBmp,
                          new MeasTreeData(MeasTreeItem::MeasItem, file_data->Index(), 0));
        continue;
      }
      auto *meas_data = meas_item.IsOk() ? dynamic_cast<MeasTreeData *>(left_->GetItemData(meas_item)) : nullptr;
      const bool expanded = left_->IsExpanded(file_item);
      if (file_data == nullptr || !meas_item.IsOk() || meas_data == nullptr || meas_data->Index() <= 0) {
        continue;
      }

      RedrawMeasList(file_item, file_data->Index());
      if (expanded) {
        left_->Expand(file_item);
      } else {
        left_->Collapse(file_item);
      }
    }
  }
}

void MeasSelectionPanel::OnLeftExpanding(wxTreeListEvent &event) {
  auto item = event.GetItem();
  if (!item.IsOk() || left_ == nullptr) {
    return;
  }
  auto child_item = left_->GetFirstChild(item);
  if (!child_item.IsOk()) {
    return;
  }
  auto *child_data = dynamic_cast<MeasTreeData *>(left_->GetItemData(child_item));
  if (child_data == nullptr) {
    return;
  }

  if (child_data->Index() == 0) {
    // Needs update from the database
    auto parent = child_data->Parent();
    auto type = child_data->Type();
    left_->DeleteItem(child_item); // Note that child_data no longer exist
    switch (type) {
      case MeasTreeItem::MeasItem:
        RedrawMeasList(item, parent);
        if (meas_list_.empty()) {
          event.Veto();
        }
        break;

      case MeasTreeItem::FileItem:
        RedrawFileList(item, parent);
        if (file_list_.empty()) {
          event.Veto();
        }
        break;

      case MeasTreeItem::TestItem:
      default:return;
    }
  }
}

void MeasSelectionPanel::OnSelectionChange(wxTreeListEvent &event) {
  auto* meas_tab = dynamic_cast<MeasurementTab*>(GetGrandParent());
  if (left_ == nullptr || meas_tab == nullptr) {
    return;
  }
  auto item = event.GetItem();
  if (!item.IsOk()) {
    meas_tab->SelectedMeas(MeasTreeData());
    return;
  }
  auto *data = dynamic_cast<MeasTreeData *>(left_->GetItemData(item));
  if (data == nullptr) {
    meas_tab->SelectedMeas(MeasTreeData());
    return;
  }
  meas_tab->SelectedMeas(*data);
}

void MeasSelectionPanel::RedrawFileList(wxTreeListItem &root_item,int64_t parent_test) {
  auto* meas_tab = dynamic_cast<MeasurementTab*>(GetGrandParent());
  if (left_ == nullptr || meas_tab == nullptr) {
    return;
  }
  meas_tab->FetchFileFromDb(parent_test);

  // Delete all child items
  for (auto del = left_->GetFirstChild(root_item); del.IsOk(); del = left_->GetFirstChild(root_item)) {
    left_->DeleteItem(del);
  }

  for (const auto& file : file_list_) {
    auto file_item = left_->AppendItem(root_item, file->Name(), kFileCloseBmp, kFileOpenBmp,
                                       new MeasTreeData(MeasTreeItem::FileItem, parent_test, file->ItemId()));

    const auto ns1970 = IsoTimeToNs(file->BaseValue<std::string>("version_date"));
    left_->SetItemText(file_item, 1, NsToLocalDate(ns1970) );

    left_->AppendItem(file_item, "Needs Update", kMeasCloseBmp, kMeasOpenBmp,
                      new MeasTreeData(MeasTreeItem::MeasItem, file->ItemId(), 0));
  }
}

void MeasSelectionPanel::RedrawMeasList(wxTreeListItem &root_item, int64_t parent_file) {
  auto* meas_tab = dynamic_cast<MeasurementTab*>(GetGrandParent());
  if (left_ == nullptr || meas_tab == nullptr) {
    return;
  }
  meas_tab->FetchMeasFromDb(parent_file);
  // Delete all child items
  for (auto del = left_->GetFirstChild(root_item); del.IsOk(); del = left_->GetFirstChild(root_item)) {
    left_->DeleteItem(del);
  }
  for (const auto& meas : meas_list_) {
    left_->AppendItem(root_item, meas->Name(),kMeasCloseBmp, kMeasOpenBmp,
                      new MeasTreeData(MeasTreeItem::MeasItem, parent_file, meas->ItemId()));
  }
}

}