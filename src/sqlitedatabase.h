/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <string>
#include <functional>
#include <sqlite3.h>
#include <util/utilfactory.h>
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

  void ConnectionInfo(const std::string& info) override;

  [[nodiscard]] const std::string& FileName() const;
  void FileName(const std::string& filename);

  bool Open() override;
  [[nodiscard]] bool OpenEx(int flags = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
  bool Close(bool commit) override;
  [[nodiscard]] bool IsOpen() const override;

  [[nodiscard]] bool Create(const IModel& model) override;

  int64_t ExecuteSql(const std::string& sql) override;

  void FetchNameMap(const ITable& table, IdNameMap &dest_list,
                    const SqlFilter& filter) override;
  void FetchItemList(const ITable& table, ItemList &dest_list,
                     const SqlFilter& filter) override;
  size_t FetchItems(const ITable &table, const SqlFilter &filter,
                  std::function<void(IItem &)> OnItem) override;
  void Vacuum() override;

  sqlite3* Sqlite3();


protected:
   [[nodiscard]] std::string DataTypeToDbString(DataType type) override;
   [[nodiscard]] bool IsDataTypeString(DataType type) override;

 private:

  sqlite3* database_ = nullptr;
  bool transaction_ = false;

  std::unique_ptr<util::log::IListen> listen_ =
      util::UtilFactory::CreateListen("ListenProxy", "LISSQLITE");
  size_t row_count_ = 0;
  int64_t exec_result_ = 0; ///< Resulting value from an ExecuteSql

  bool ReadSvcEnumTable(IModel& model) override;
  bool ReadSvcEntTable(IModel& model) override;
  bool ReadSvcAttrTable(IModel& model) override;
  bool FetchModelEnvironment(IModel& model) override;

  static int TraceCallback(unsigned mask, void* context,  void* arg1,
                           void* arg2);
  static int ExecCallback(void* object, int rows, char** value_list,
                          char** column_list);
};

} // end namespace




