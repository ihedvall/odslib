/*
* Copyright 2022 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "meastreedata.h"
namespace ods::gui {

MeasTreeData::MeasTreeData(MeasTreeItem type, int64_t parent, int64_t index)
: type_(type),
  parent_(parent),
  index_(index) {

}

} // end namespace