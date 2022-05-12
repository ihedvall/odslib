/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include "ods/iitem.h"
#include <util/stringutil.h>
#include <util/timestamp.h>

using namespace util::string;
using namespace util::time;

namespace ods {

IItem::IItem(const std::string &app_name)
: application_name_(app_name) {

}

IItem::IItem(int64_t app_id)
    : application_id_(app_id) {
}

const IAttribute *IItem::GetAttribute(const std::string &name) const {
  if (name.empty()) {
    return nullptr;
  }
  const auto itr = std::ranges::find_if(attribute_list_, [&] (const auto& item) {
    return IEquals(name, item.Name());
  });
  return itr == attribute_list_.cend() ? nullptr : &*itr;
}

const IAttribute *IItem::GetBaseAttribute(const std::string &base_name) const {
  if (base_name.empty()) {
    return nullptr;
  }
  const auto itr = std::ranges::find_if(attribute_list_, [&] (const auto& item) {
    return IEquals(base_name, item.BaseName());
  });
  return itr == attribute_list_.cend() ? nullptr : &*itr;
}

bool IItem::ExistAttribute(const std::string &name) const {
  if (name.empty()) {
    return false;
  }
  return std::ranges::any_of(attribute_list_, [&] (const auto& item) {
    return IEquals(name, item.Name());
  });
}

bool IItem::ExistBaseAttribute(const std::string &base_name) const {
  if (base_name.empty()) {
    return false;
  }
  return std::ranges::any_of(attribute_list_, [&] (const auto& item) {
    return IEquals(base_name, item.BaseName());
  });
}

int64_t IItem::ApplicationId() const {
  return application_id_;
}

void IItem::ApplicationId(int64_t ident) {
  application_id_ = ident;
}

int64_t IItem::ItemId() const {
  if (item_id_ <= 0) {
    // Fetch it from the base name 'id' column.
    const auto* id_col = GetBaseAttribute("id");
    if (id_col != nullptr) {
      return id_col->Value<int64_t>();
    }
  }
  return item_id_;
}

void IItem::ItemId(int64_t index) {
  item_id_ = index;
  auto itr = std::ranges::find_if( attribute_list_, [] (const auto& attr) {
    return IEquals(attr.BaseName(), "id");
  });
  if (itr != attribute_list_.end()) {
    itr->Value(index);
  }
}

std::string IItem::Name() const {
  const auto* name_col = GetBaseAttribute("name");
  return name_col == nullptr ? std::string() : name_col->Value<std::string>();
}

uint64_t IItem::Created() const {
  const auto* created_col = GetBaseAttribute("ao_created");
  return created_col == nullptr ? 0 : IsoTimeToNs(created_col->Value<std::string>());
}

uint64_t IItem::Modified() const {
  const auto* modified_col = GetBaseAttribute("ao_last_modified");
  return modified_col == nullptr ? 0 : IsoTimeToNs(modified_col->Value<std::string>());
}

const std::string &IItem::ApplicationName() const {
  return application_name_;
}

void IItem::ApplicationName(const std::string &name) {
  application_name_ = name;
}

void IItem::AppendAttribute(const IAttribute &attribute) {
  attribute_list_.push_back(attribute);
}

void IItem::SetAttribute(const IAttribute &attribute) {
  auto itr = std::ranges::find_if(attribute_list_, [&](const auto& attr ) {
    return IEquals(attribute.Name(), attr.Name());
  });
  if (itr != attribute_list_.end()) {
    itr->Value(attribute.Value<std::string>());
  } else {
    attribute_list_.push_back(attribute);
  }
}

const std::vector<IAttribute> &IItem::AttributeList() const {
  return attribute_list_;
}

std::vector<IAttribute> &IItem::AttributeList() {
  return attribute_list_;
}

} // end namespace