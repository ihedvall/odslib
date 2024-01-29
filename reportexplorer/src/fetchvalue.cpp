/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include <util/logstream.h>
#include "fetchvalue.h"
#include "reportexplorer.h"
#include <mdf/idatagroup.h>

using namespace util::log;
using namespace mdf;

namespace ods::gui {

FetchValue::~FetchValue() {
  subscription_list_.clear();
  reader_list_.clear();
}

void FetchValue::AddItem(const IItem &item) {
  const auto item_exist = std::ranges::any_of(subscription_list_, [&] (const auto& sub) {
    return sub && sub->item.ItemId() == item.ItemId();
  });
  if (item_exist) {
    return;
  }
  // Find the MDF file and add a reader for it
  const auto file_index = item.Value<int64_t>("TestFile");
  auto& app = wxGetApp();
  const auto filename = app.FetchFile(file_index);
  if (filename.empty()) {
    LOG_ERROR() << "Failed to find the data file. Index: " << file_index;
    return;
  }

  auto file_itr = std::ranges::find_if(reader_list_, [&] (const auto& file) {
    return file && file->Index() == file_index;
  });

  MdfReader* reader = nullptr;
  if (file_itr == reader_list_.end()) {
    // Add a new reader
    auto new_reader = std::make_unique<MdfReader>(filename);
    new_reader->Index(file_index);
    new_reader->ReadEverythingButData();
    reader = new_reader.get();
    reader_list_.push_back(std::move(new_reader));
  } else {
    reader = file_itr->get();
  }
  if (reader == nullptr) {
    return;
  }

  const auto dg_index = item.Value<size_t>("DgIndex");
  auto* dg_group = const_cast<IDataGroup*>(reader->GetDataGroup(dg_index));
  if (dg_group == nullptr) {
    return;
  }
  dg_group->SetAsRead(false);

  auto subscription = std::make_unique<ChannelSubscription>();
  subscription->item = item;
  subscription->observer = std::move(CreateChannelObserver(*dg_group, item.Name()));
  subscription->data_group = dg_group;
  subscription->reader = reader;

  subscription_list_.push_back(std::move(subscription));

}

void FetchValue::ReadValues() {
  for (auto& sub : subscription_list_) {
    if (!sub || sub->reader == nullptr || !sub->reader->IsOk()
             || sub->data_group == nullptr || !sub->observer) {
      continue;
    }
    // Check if reading is necessary
    if (sub->data_group->IsRead()) {
      // Already read by earlier calls
      continue;
    }
    sub->reader->ReadData(*sub->data_group);
    sub->data_group->SetAsRead(true);
  }
}


void FetchValue::ExportToCsv(util::plot::CsvWriter &csv) const {
  std::vector<int> column_order; // index into the subscription list with X, Y1..Y2:: order. -1 = samples
  const auto& list = subscription_list_;
  // Add X-axis first
  int x_index = -1;
  for (size_t x_count = 0; x_count < list.size(); ++x_count) {
    if (list[x_count]->item.Value<std::string>("Selected") == "X") {
      x_index = static_cast<int>(x_count);
      break;
    }
  }
  column_order.push_back(x_index);

  // Add Y1-axis next
  for (size_t y1_count = 0; y1_count < list.size(); ++y1_count) {
    if (list[y1_count]->item.Value<std::string>("Selected") == "Y1") {
      column_order.push_back(static_cast<int>(y1_count));
    }
  }

  // Add Y2-axis last
  for (size_t y2_count = 0; y2_count < list.size(); ++y2_count) {
    if (list[y2_count]->item.Value<std::string>("Selected") == "Y2") {
      column_order.push_back(static_cast<int>(y2_count));
    }
  }

  // Add header to CSV
  for (int sub_index : column_order) {
    if (sub_index < 0 || sub_index >= list.size()) {
      csv.AddColumnHeader("Sample", "", true);
    } else {
      auto& observer = list[sub_index]->observer;
      if (observer) {
        csv.AddColumnHeader(observer->Name(), observer->Unit(), false);
      } else {
        csv.AddColumnHeader("Unknown", "", false);
      }
    }
  }
  csv.AddRow();

  bool more_samples = true;

  for (size_t sample_count = 0; more_samples; ++sample_count) {
    more_samples = false;

    for (size_t column = 0; column < column_order.size(); ++column) {
      const int column_index = column_order[column];
      const IChannelObserver* observer = nullptr;
      if (column_index >= 0 && column_index < list.size()) {
        observer = list[column_index]->observer.get();
      }
      if (column_index < 0) {
        csv.AddColumnValue(sample_count);
        csv.SetColumnValid(column);
      } else if (observer == nullptr || sample_count >= observer->NofSamples()) {
        csv.AddColumnValue(std::string());
      } else {
        if (sample_count + 1 < observer->NofSamples()) {
          more_samples = true;
        }
        std::string value;
        auto valid = observer->GetEngValue(sample_count, value);
        if (util::string::IEquals(value, "NaN")) {
          valid = false;
        }
        if (valid) {
          csv.AddColumnValue(value);
          csv.SetColumnValid(column);
        } else {
          csv.AddColumnValue(std::string("NaN"));
        }
      }
    }
    csv.AddRow();
  }


}


} // end namespace