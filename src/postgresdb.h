/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include "ods/idatabase.h"
#include <libpq-fe.h>
#include <util/ilisten.h>
#include <string>
#include <memory>

namespace ods::detail {

class PostgresDb : public IDatabase {
public:
  PostgresDb();
  PostgresDb(const PostgresDb&) = delete;

  ~PostgresDb() override;
  PostgresDb& operator = (const PostgresDb&) = delete;

  bool Open() override;
  bool Close(bool commit) override;
  [[nodiscard]] bool IsOpen() const override;

  int64_t ExecuteSql(const std::string& sql) override;

  void FetchNameMap(const ITable& table, IdNameMap &dest_list,
                    const SqlFilter& filter) override;
  void FetchItemList(const ITable& table, ItemList &dest_list,
                     const SqlFilter& filter) override;

  PGconn* Connection() {return connection_;}
  size_t FetchItems(const ITable &table, const SqlFilter &filter,
                    std::function<void(IItem &)> OnItem) override;

protected:
  [[nodiscard]] std::string DataTypeToDbString(DataType type) override;
  [[nodiscard]] bool IsDataTypeString(DataType type) override;

  bool ReadSvcEnumTable(IModel& model) override;
  bool ReadSvcEntTable(IModel& model) override;
  bool ReadSvcAttrTable(IModel& model) override;
  bool ReadSvcRefTable(IModel& model) override;

  bool FetchModelEnvironment(IModel& model) override;
private:
  PGconn* connection_ = nullptr;
  std::unique_ptr<util::log::IListen> listen_;

  bool HandleConnectionStringError();
  bool HandleConnectionError();


};

} // namespace ods::detail
