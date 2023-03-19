/*
* Copyright 2022 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <wx/clntdata.h>

namespace ods::gui {
enum class MeasTreeItem : int {
  TestItem = 0,
  FileItem = 1,
  MeasItem = 2,
  ChannelItem = 3
};

class MeasTreeData : public wxClientData {
 public:
  MeasTreeData() = default;
  ~MeasTreeData() override = default;
  MeasTreeData(MeasTreeItem type,  int64_t parent, int64_t index);

  [[nodiscard]] MeasTreeItem Type() const {
    return type_;
  }

  [[nodiscard]] int64_t Parent() const {
    return parent_;
  }

  [[nodiscard]] int64_t Index() const {
    return index_;
  }

 private:
  MeasTreeItem type_ = MeasTreeItem::TestItem;
  int64_t parent_ = 0;
  int64_t index_ = 0;
};

} // end namespace
