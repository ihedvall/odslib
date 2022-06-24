/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <string>
#include <sqlite3.h>
#include <util/ilisten.h>
#include "ods/idatabase.h"
#include "ods/imodel.h"
#include "ods/iitem.h"
#include "ods/sqlfilter.h"

namespace ods::detail {


class SqliteDatabase : public IDatabase {
 public:
  SqliteDatabase() = default;
  explicit SqliteDatabase(const std::string& filename);
  ~SqliteDatabase() override;

  [[nodiscard]] const std::string& FileName() const;
  void FileName(const std::string& filename);

  [[nodiscard]] std::string Name() const;

  bool Open() override;
  [[nodiscard]] bool OpenEx(int flags = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
  bool Close(bool commit) override;
  [[nodiscard]] bool IsOpen() const override;

  [[nodiscard]] bool Create(IModel& model) override;
  [[nodiscard]] bool ReadModel(IModel& model) override;

  void Insert(const ITable& table, IItem& row) override;
  void Update(const ITable& table, IItem& row, const SqlFilter& filter) override;
  void Delete(const ITable& table, const SqlFilter& filter) override;
  void ExecuteSql(const std::string& sql) override;
  void FetchNameMap(const ITable& table, IdNameMap &dest_list, const SqlFilter& filter) override;
  void FetchItemList(const ITable& table, ItemList &dest_list, const SqlFilter& filter) override;

  sqlite3* Sqlite3();

 private:
  std::string filename_;
  sqlite3* database_ = nullptr;
  bool transaction_ = false;

  std::unique_ptr<util::log::IListen> listen_ = util::log::IListen::CreateListen("ListenProxy", "LISSQLITE");
  size_t row_count_ = 0;

  bool CreateSvcEnumTable(IModel &model);
  bool CreateSvcEntTable(const IModel &model);
  bool CreateSvcAttrTable(const IModel &model);
  bool CreateTables(const IModel& model);
  bool InsertModelUnits(const IModel& model);
  bool InsertModelEnvironment(const IModel& model);

  bool ReadSvcEnumTable(IModel& model);
  bool ReadSvcEntTable(IModel& model);
  bool ReadSvcAttrTable(IModel& model);
  bool FixUnitStrings(const IModel& model);
  bool FetchModelEnvironment(IModel& model);

  static int TraceCallback(unsigned mask, void* context,  void* arg1, void* arg2);
};

} // end namespace




