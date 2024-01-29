/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include <vector>
#include <util/csvwriter.h>
#include <mdf/mdfreader.h>
#include <ods/iitem.h>


namespace ods::gui {

struct ChannelSubscription {
  IItem item;
  mdf::ChannelObserverPtr observer;
  mdf::IDataGroup* data_group = nullptr;
  mdf::MdfReader* reader = nullptr;
};

/**
 * Simple helper class for fetching channel values from data files in a efficient way.
 */
class FetchValue {
 public:
  virtual ~FetchValue();
  void AddItem(const IItem& item);
  void ReadValues();
  void ExportToCsv(util::plot::CsvWriter& csv) const;
 private:
  std::vector<std::unique_ptr<ChannelSubscription>> subscription_list_;
  std::vector<std::unique_ptr<mdf::MdfReader>> reader_list_;
};

} // end namespace