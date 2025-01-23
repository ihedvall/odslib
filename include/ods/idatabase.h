/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include <functional>
#include <map>

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

[[nodiscard]] bool IsSqlReservedWord(const std::string& word);
[[nodiscard]] std::string MakeBlobString(const std::vector<uint8_t>& blob);

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

  virtual void Insert(const ITable& table, IItem& row,
                      const SqlFilter& filter);
  virtual void Update(const ITable& table, IItem& row,
                      const SqlFilter& filter);
  virtual void Delete(const ITable& table, const SqlFilter& filter);

  virtual int64_t ExecuteSql(const std::string& sql) = 0;
  virtual size_t Count(const ITable& table, const SqlFilter& filter);

  virtual bool ExistDatabaseTable(const std::string& dbt_name);

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

  std::string DumpDatabase(const std::string& root_dir);
  bool ReadInDump(const std::string& dump_dir);

 protected:
  bool use_indexes_ = true;  ///< Flag that enable/disable automatic increment indexes;
  bool use_constraints_ = true; ///< Flag that enables/disables constraints checks
  IDatabase() = default;

  void  DatabaseType(DbType type ) {type_of_database_ = type;}

  bool CreateSvcEnumTable(const IModel &model);
  bool CreateSvcEntTable(const IModel &model);
  bool CreateSvcAttrTable(const IModel &model);
  bool CreateSvcRefTable(const IModel &model);

  bool CreateTables(const IModel& model);
  bool CreateRelationTables(const IModel& model);

  std::string MakeCreateTableSql(const IModel& model,
                                const ITable& table);
  bool InsertModelEnvironment(const IModel& model);
  bool InsertModelUnits(const IModel& model);
  bool FixUnitStrings(const IModel& model);

  virtual bool ReadSvcEnumTable(IModel& model) = 0;
  virtual bool ReadSvcEntTable(IModel& model) = 0;
  virtual bool ReadSvcAttrTable(IModel& model) = 0;
  virtual bool ReadSvcRefTable(IModel& model) = 0;
  virtual bool FetchModelEnvironment(IModel& model) = 0;

  [[nodiscard]] virtual std::string DataTypeToDbString(DataType type) = 0;
  [[nodiscard]] virtual bool IsDataTypeString(DataType type) = 0;

  /** \brief Converts an attribute value to a SQL string.
   *
   * This function is database dependent and converts an attribute
   * ISO timestamp string into the database timestamp string.
   * By default this function convert to a SQLite string which
   * happens to be just the attribute string value with pre/post
   * dots.
   * @param attr Column attribute object
   * @return SQL string that the database can use
   */
  [[nodiscard]] virtual std::string MakeDateValue(const IAttribute& attr) const;

  [[nodiscard]] virtual bool DumpTable(const std::string& dump_dir, const ITable& table);
  [[nodiscard]] virtual bool DumpRow(const ITable& table, const IItem& row, std::ofstream& out_file) const;
  /** \brief Specialized insert command for inserting dump row.
   *
   * The function inserts dump file row.
   * When inserting form a dump, the automatic indexes and constraints are
   * disabled.
   *
   * Note that the databases should override this function as the datatypes differs
   * between the databases. The BLOB and string columns should also be bound.
   * @param table Ods table object.
   * @param row Data values to insert into the database
   */
  virtual void InsertDumpRow(const ITable& table, IItem& row);

  virtual void EnableIndexing(bool enable);
  virtual void EnableConstraints(bool enable);

 private:
  DbType type_of_database_ = DbType::TypeGeneric;
  std::string name_; ///< Database name
  std::string connection_info_; ///< Connection string or file name

  void AddComments(const ITable& table);
  [[nodiscard]] std::string CreateDumpDir(const std::string& root_dir) const;
  [[nodiscard]] bool SaveModelFile(const std::string& dump_dir, const IModel& model) const;
  [[nodiscard]] static bool ReadInDumpFiles(const std::string& dump_dir, std::string& model_file,
                       std::map<std::string, std::string>& dbt_list);
  [[nodiscard]] bool IsEmpty(const IModel& model);
  [[nodiscard]] bool ReadInData(const IModel& model, const std::map<std::string, std::string>& dbt_list);
  [[nodiscard]] bool ReadInTable(const ITable& table, const std::string& dbt_file);


};

} // end namespace ods
