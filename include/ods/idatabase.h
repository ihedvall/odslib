/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include "ods/itable.h"
#include "ods/iitem.h"
#include "ods/sqlfilter.h"

namespace ods {

bool IsSqlReservedWord(const std::string& word);

class IDatabase {
 public:
  virtual ~IDatabase() = default;
//  virtual std::string Name() const = 0;

  virtual bool Open() = 0;
  virtual bool Close(bool commit = true) = 0;
  virtual bool IsOpen() const = 0;

  virtual void Insert(const ITable& table, IItem& row) = 0;
  virtual void Update(const ITable& table, IItem& row, const SqlFilter& filter) = 0;
  virtual void Delete(const ITable& table, const SqlFilter& filter) = 0;
  virtual void ExecuteSql(const std::string& sql) = 0;

 protected:
  IDatabase() = default;
 private:

};

} // end namespace ods
