/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include "ods/itable.h"
#include "ods/iitem.h"
#include "ods/sqlfilter.h"
#include "ods/imodel.h"

namespace ods {

bool IsSqlReservedWord(const std::string& word);

class IDatabase {
 public:
  virtual ~IDatabase() = default;

  virtual bool Open() = 0;
  virtual bool Close(bool commit) = 0;
  [[nodiscard]] virtual bool IsOpen() const = 0;

  [[nodiscard]] virtual bool Create(IModel& model) = 0;
  [[nodiscard]] virtual bool ReadModel(IModel& model) = 0;

  virtual void Insert(const ITable& table, IItem& row) = 0;
  virtual void Update(const ITable& table, IItem& row, const SqlFilter& filter) = 0;
  virtual void Delete(const ITable& table, const SqlFilter& filter) = 0;
  virtual void ExecuteSql(const std::string& sql) = 0;
  virtual void FetchNameMap(const ITable& table, IdNameMap &dest_list, const SqlFilter& filter) = 0;
  virtual void FetchItemList(const ITable& table, ItemList &dest_list, const SqlFilter& filter ) = 0;
 protected:
  IDatabase() = default;
 private:

};

} // end namespace ods
