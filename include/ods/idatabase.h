/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <functional>
#include "ods/itable.h"
#include "ods/iitem.h"
#include "ods/sqlfilter.h"
#include "ods/imodel.h"

namespace ods {

enum class DbType : uint8_t {
  TypeGeneric = 0,
  TypeSqlite = 1,
  TypePostgres = 2,
  TypeOracle = 3,
  TypeSqlServer = 4
};

bool IsSqlReservedWord(const std::string& word);
[[maybe_unused]] std::string MakeBlobString(const std::vector<uint8_t>& blob);

class IDatabase {
 public:
  virtual ~IDatabase() = default;

  [[nodiscard]] DbType DatabaseType() const { return type_of_database_;}
  /** \brief Returns the database type as a string. */
  [[nodiscard]] std::string DatabaseTypeAsString() const;
  /** \brief Function that convert a string to a database type. */
  [[nodiscard]] static DbType StringAsDatabaseType(const std::string& type);

  void Name(const std::string& name) {name_ = name;}
  [[nodiscard]] const std::string& Name() const {return name_;}

  virtual void ConnectionInfo(const std::string& info);

  [[nodiscard]] const std::string& ConnectionInfo() const {
    return connection_info_;
  }

  virtual bool Open() = 0;
  virtual bool Close(bool commit) = 0;
  [[nodiscard]] virtual bool IsOpen() const = 0;

  [[nodiscard]] virtual bool Create(const IModel& model);
  [[nodiscard]] virtual bool ReadModel(IModel& model);

  /** \brief Returns a row index if item already exists.
   *
   * Return a database index if the item in the where
   * statement exists. The SQL filter object may not be empty.
   * @param table Reference to the database table.
   * @param filter Reference to the filter with the where statement.
   * @return The existing index or zero if not found.
   */
  [[nodiscard]] int64_t Exists(const ITable& table, const SqlFilter& filter);

  void Insert(const ITable& table, IItem& row,
                      const SqlFilter& filter);
  void Update(const ITable& table, IItem& row,
                      const SqlFilter& filter);
  virtual void Delete(const ITable& table, const SqlFilter& filter);
  virtual int64_t ExecuteSql(const std::string& sql) = 0;
  virtual size_t Count(const ITable& table, const SqlFilter& filter);

  virtual void FetchNameMap(const ITable& table, IdNameMap &dest_list,
                            const SqlFilter& filter) = 0;
  virtual void FetchItemList(const ITable& table, ItemList &dest_list,
                             const SqlFilter& filter ) = 0;
  /** \brief Fetch row by row and calls the onItem() function for each row.
   *
   * Function that is used to fetch row by row instead of allocating a list of
   * all rows. This is typically used in RPC servers with a streaming
   * interface where no list is needed afterward.
   *
   * @param table Reference to the table and its columns.
   * @param filter Reference to the where filtering definition..
   * @param OnItem Called for each row in the select statement.
   * @returns Number of rows handled in call.
   */
  virtual size_t FetchItems(const ITable& table, const SqlFilter& filter,
                          std::function<void(IItem&)> OnItem  ) = 0;

    /** \brief Optimize the database in size and performance.
     *
     * Function that optimize size and performance of the database. Note that
     * this function only exist in some databases so by default this function doesn't do anything.
     *
     * Note that the function open and closes the database inside this function, so ensure
     * that the database is closed before calling the function. The function may block
     * the database for some time.
     */
  virtual void Vacuum();
  virtual void ExportCsv(const std::string& filename, const ITable& table,
                         const SqlFilter& filter);
 protected:
  IDatabase() = default;

  void  DatabaseType(DbType type ) {type_of_database_ = type;}

  bool CreateSvcEnumTable(const IModel &model);
  bool CreateSvcEntTable(const IModel &model);
  bool CreateSvcAttrTable(const IModel &model);
  bool CreateTables(const IModel& model);
  std::string MakeCreateTableSql(const IModel& model,
                                const ITable& table);
  bool InsertModelEnvironment(const IModel& model);
  bool InsertModelUnits(const IModel& model);
  bool FixUnitStrings(const IModel& model);

  virtual bool ReadSvcEnumTable(IModel& model) = 0;
  virtual bool ReadSvcEntTable(IModel& model) = 0;
  virtual bool ReadSvcAttrTable(IModel& model) = 0;
  virtual bool FetchModelEnvironment(IModel& model) = 0;

  [[nodiscard]] virtual std::string DataTypeToDbString(DataType type) = 0;
  [[nodiscard]] virtual bool IsDataTypeString(DataType type) = 0;

 [[nodiscard]] virtual std::string MakeDateValue(const IAttribute& attr) const;

 private:
  DbType type_of_database_ = DbType::TypeGeneric;
  std::string name_; ///< Database name
  std::string connection_info_; ///< Connection string or file name

  void AddComments(const ITable& table);
};

} // end namespace ods
