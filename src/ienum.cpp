/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "ods/ienum.h"

namespace ods {

void IEnum::AddItem(int64_t index, const std::string &item) {
  auto itr = item_list_.find(index);
  if (itr == item_list_.end()) {
    item_list_.insert({index, item});
  } else {
    itr->second = item;
  }
}

void IEnum::DeleteItem(int64_t index) {
  auto itr = item_list_.find(index);
  if (itr != item_list_.end()) {
    item_list_.erase(itr);
  }
}

}