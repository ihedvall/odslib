/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <string_view>
#include <algorithm>

#include "util/logstream.h"
#include "util/stringutil.h"
#include "util/timestamp.h"
#include "ods/databaseguard.h"
#include "ods/baseattribute.h"
#include "sqlitedatabase.h"
#include "sqlitestatement.h"

using namespace std::chrono_literals;
using namespace util::log;
using namespace util::string;
using namespace util::time;
namespace {

constexpr std::string_view kCreateSvcEnum = "CREATE TABLE IF NOT EXISTS SVCENUM ("
  "ENUMID  INTEGER NOT NULL, "
  "ENUMNAME TEXT NOT NULL, "
  "ITEM INTEGER NOT NULL, "
  "ITEMNAME TEXT, "
  "LOCKED INTEGER ck_locked CHECK (LOCKED IN (0,1)) DEFAULT 1, "
  "CONSTRAINT pk_svcenum PRIMARY KEY (ENUMID,ITEM) )";

constexpr std::string_view kInsertSvcEnum = "INSERT OR REPLACE INTO SVCENUM ("
  "ENUMID,ENUMNAME ,ITEM, ITEMNAME, LOCKED) VALUES (%lld,%Q, %lld,%Q, %d)";

constexpr std::string_view kCreateSvcEnt = "CREATE TABLE IF NOT EXISTS SVCENT ("
                                            "AID INTEGER pk_svcent PRIMARY KEY NOT NULL, "
                                            "ANAME TEXT uq_svcent NOT NULL UNIQUE, "
                                            "BID INTEGER NOT NULL, "
                                            "DBTNAME TEXT, "
                                            "SECURITY INTEGER DEFAULT 0, "
                                            "DESC TEXT)";

constexpr std::string_view kInsertSvcEnt = "INSERT OR REPLACE INTO SVCENT ("
          "AID, ANAME ,BID ,DBTNAME, SECURITY, DESC) VALUES (%lld, %Q, %d, %Q, %lld, %Q)";

constexpr std::string_view kCreateSvcAttr = "CREATE TABLE IF NOT EXISTS SVCATTR ("
              "AID INTEGER NOT NULL, "
              "ATTRNR INTEGER, "
              "AANAME TEXT NOT NULL, "
              "BANAME TEXT, "
              "FAID INTEGER, "
              "FUNIT INTEGER, "
              "ADTYPE INTEGER, "
              "AFLEN INTEGER, "
              "DBCNAME TEXT, "
              "ACLREF INTEGER DEFAULT 0, "
              "INVNAME TEXT, "
              "FLAG INTEGER, "
              "ENUMNAME TEXT, "
              "DESC TEXT, "
              "DISPNAME TEXT, "
              "NOFDEC INTEGER, "
              "DEFVALUE TEXT, "
              "CONSTRAINT pk_svcattr PRIMARY KEY (AID,AANAME))";

constexpr std::string_view kInsertSvcAttr = "INSERT OR REPLACE INTO SVCATTR ("
 "AID,ATTRNR,AANAME,BANAME,FAID,FUNIT,ADTYPE,AFLEN,DBCNAME,ACLREF,INVNAME,FLAG,ENUMNAME,DESC,DISPNAME,NOFDEC,DEFVALUE) "
 "VALUES (%lld, %lld, %Q, %Q, %s, %s, %d, %d, %Q, %lld, %Q, %d, %Q, %Q, %Q, %d, %Q)";


int BusyHandler(void* user, int nof_locks) {
  if (nof_locks < 1000) {
    std::this_thread::sleep_for(10ms);
    return 1;
  }
  return 0;
}

std::string DataTypeToDbString(ods::DataType type) {
  switch (type) {
    case ods::DataType::DtShort:
    case ods::DataType::DtBoolean:
    case ods::DataType::DtByte:
    case ods::DataType::DtLong:
    case ods::DataType::DtLongLong:
    case ods::DataType::DtId:
    case ods::DataType::DtEnum:
      return "INTEGER";

    case ods::DataType::DtDouble:
    case ods::DataType::DtFloat:
      return "REAL";

    case ods::DataType::DtByteString:
    case ods::DataType::DtBlob:
      return "BLOB";

    default:break;
  }
  return "TEXT";
}

bool IsDataTypeString(ods::DataType type) {
  return DataTypeToDbString(type) == "TEXT";
}

std::string MakeCreateTableSql(const ods::IModel& model, const ods::ITable& table) {
  const auto& column_list = table.Columns();
  const auto unique_list = table.MakeUniqueList();

  std::ostringstream sql;
  sql << "CREATE TABLE IF NOT EXISTS " << table.DatabaseName() << " (";
  // Add primary key first (always the id column
  for (const auto& iid : column_list) {
    if (iid.DatabaseName().empty() || !IEquals(iid.BaseName(), "id")) {
      continue;
    }
    sql << iid.DatabaseName() << " INTEGER PRIMARY KEY AUTOINCREMENT";
    break;
  }

  // Add the other columns
  for (const auto& column : column_list) {

    if (column.DatabaseName().empty() || IEquals(column.BaseName(), "id")) {
      continue;
    }
    sql << "," << std::endl;
    sql << column.DatabaseName() << " " << DataTypeToDbString(column.DataType());
    if (column.Obligatory()) {
      sql << " NOT NULL";
    }

    if (column.Unique() && unique_list.size() <= 1) {
      sql << " UNIQUE";
    }

    if (!column.DefaultValue().empty()) {
      sql << " DEFAULT ";
      if (IsDataTypeString(column.DataType())) {
        auto* temp = sqlite3_mprintf( "%Q", column.DefaultValue().c_str());
        sql << temp;
        sqlite3_free(temp);
      } else {
        sql << column.DefaultValue();
      }
    }

    if (!column.CaseSensitive() && column.Unique() && IsDataTypeString(column.DataType()) ) {
      sql << " COLLATE NOCASE";
    }

    const auto* ref_table = column.ReferenceId() > 0 ? model.GetTable(column.ReferenceId()) : nullptr;
    if (ref_table != nullptr ) {
      sql << " REFERENCES " << ref_table->DatabaseName();
      if (!column.ReferenceName().empty()) {
        sql << "(" << column.ReferenceName() << ")";
      }
      if (column.Obligatory()) {
        sql << " ON DELETE CASCADE";
      } else {
        sql << " ON DELETE SET NULL";
      }
    }
  }

  // Done with the individual columns. Now it's time for unique constraint
  if (unique_list.size() > 1) {
    sql << ", CONSTRAINT UQ_" << table.DatabaseName() << " UNIQUE(";
    for(size_t unique = 0; unique < unique_list.size(); ++unique) {
      const auto& column = unique_list[unique];
      if (unique > 0) {
        sql << ",";
      }
      sql << column.DatabaseName();
    }
    sql << ")";
  }

  sql << ")";

  return sql.str();
}

} // end namespace

namespace ods::detail {

SqliteDatabase::SqliteDatabase(const std::string &filename) {
  FileName(filename);
}

SqliteDatabase::~SqliteDatabase() {
  Close(false);
}

bool SqliteDatabase::Open() {
  bool open = false;
  try {
    if (database_ == nullptr) {
      if (!std::filesystem::exists(filename_) ) {
        std::ostringstream err;
        err << "Database file does not exist. File: " << filename_;
        throw std::runtime_error(err.str());
      }

      int open3;
      size_t locks = 0;
      for (open3 = sqlite3_open_v2(filename_.c_str(), &database_, SQLITE_OPEN_READWRITE, nullptr);
           open3 == SQLITE_BUSY && locks < 1000;
           open3 = sqlite3_open(filename_.c_str(), &database_) ) {
          std::this_thread::sleep_for(10ms);
      }

      if (open3 != SQLITE_OK) {
        if (database_ != nullptr) {
          const auto error = sqlite3_errmsg(database_);
          sqlite3_close_v2(database_);
          database_ = nullptr;
          LOG_ERROR() << "Failed to open the database. Error: " << error << ", File: " << filename_;
        } else {
          LOG_ERROR() << "Failed to open the database. File: " << filename_;
        }
      }
    }

    if (database_ != nullptr) {
      sqlite3_busy_handler(database_,BusyHandler, this);
      ExecuteSql("PRAGMA foreign_keys = ON");
      ExecuteSql("BEGIN TRANSACTION");
      transaction_ = true;
      open = true;
    }
  } catch (const std::exception& error) {
    Close(false);
    LOG_ERROR() << error.what();
  }
  return open;
}

bool SqliteDatabase::OpenEx(int flags) {
  const auto open3 = sqlite3_open_v2(filename_.c_str(), &database_, flags, nullptr);
  if (open3 != SQLITE_OK) {
    if (database_ != nullptr) {
      const auto error = sqlite3_errmsg(database_);
      sqlite3_close_v2(database_);
      database_ = nullptr;
      LOG_ERROR() << "Failed to open the database. Error: " << error << ", File: " << filename_;
    } else {
      LOG_ERROR() << "Failed to open the database. File: " << filename_;
    }
  }
  if (database_ != nullptr) {
    sqlite3_busy_handler(database_,BusyHandler, this);
    ExecuteSql("PRAGMA foreign_keys = ON");
    ExecuteSql("BEGIN TRANSACTION");
    transaction_ = true;
  }
  return open3 == SQLITE_OK;
}

bool SqliteDatabase::Close(bool commit) {
  if (database_ == nullptr) {
    return true;
  }
  if (transaction_) {
    try {
      ExecuteSql(commit ? "COMMIT" : "ROLLBACK");
    } catch (const std::exception& error) {
      LOG_ERROR() << "Ending transaction failed. Error:" << error.what();
    }
    transaction_ = false;
  }

  const auto close = sqlite3_close_v2(database_);
  if (close != SQLITE_OK && database_ != nullptr) {
    LOG_ERROR() << "Failed to close the database. Error: " << sqlite3_errmsg(database_) << ", File: " << filename_;
  }
  database_ = nullptr;
  return close == SQLITE_OK;
}

bool SqliteDatabase::IsOpen() const {
  return database_ != nullptr;
}

void SqliteDatabase::ExecuteSql(const std::string &sql) {
  if (database_ == nullptr) {
    throw std::runtime_error("Database not open");
  }
  char* error = nullptr;
  const auto exec = sqlite3_exec(database_, sql.c_str(), nullptr, nullptr, &error);
  if (error != nullptr) {
    std::ostringstream err;
    err << "SQL Execute error. Error: " << error << ", SQL:" << sql;
    sqlite3_free(error);
    LOG_ERROR() << err.str();
    throw std::runtime_error(err.str());
  }
}

sqlite3 *SqliteDatabase::Sqlite3() {
  return database_;
}

const std::string &SqliteDatabase::FileName() const {
  return filename_;
}

void SqliteDatabase::FileName(const std::string &filename) {
  // Seems that the SQLITE is sensible about the slashes
  try {
    std::filesystem::path file(filename);
    file.make_preferred();
    filename_ = file.string();
  } catch (const std::exception& error) {
    LOG_ERROR() << "File name path error. Error: " << error.what()
      << ", File Name: " << filename;
  }
}

bool SqliteDatabase::Create(IModel &model) {
  const auto open = OpenEx(); // Special open that creates the file
  if (!open) {
    LOG_ERROR() << "Failed to create an empty SQLITE database. DB: " << FileName();
    return false;
  }
  const auto svc_enum = CreateSvcEnumTable(model);
  const auto svc_ent = CreateSvcEntTable(model);
  const auto svc_attr = CreateSvcAttrTable(model);
  const auto tables = CreateTables(model);
  const auto units = InsertModelUnits(model);
  const auto env = InsertModelEnvironment(model);
  const auto close = Close(true);
  return close && svc_enum && svc_ent && svc_attr && tables && units && env;
}

bool SqliteDatabase::ReadModel(IModel &model) {
  DatabaseGuard guard(*this);
  if (!guard.IsOk()) {
    LOG_ERROR() << "Failed to open the SQLITE database. DB: " << FileName();
    return false;
  }

  const auto svc_enum = ReadSvcEnumTable(model);
  const auto svc_ent = ReadSvcEntTable(model);
  const auto svc_attr = ReadSvcAttrTable(model);
  const auto units = FixUnitStrings(model);
  const auto env = FetchModelEnvironment(model);

  return svc_enum && svc_ent && svc_attr && units && env;
}

bool SqliteDatabase::CreateSvcEnumTable(IModel &model) {
  try {
    ExecuteSql(kCreateSvcEnum.data());
    auto& enum_list = model.Enums();

    // Insert enumerates
    for (auto& itr : enum_list) {
      auto& obj = itr.second;
      if (obj.EnumId() <= 0) {
        obj.EnumId(model.FindNextEnumId());
      }
      if (obj.Items().empty()) {
        obj.AddItem(0,"");
      }
      for ( const auto& item : obj.Items()) {
        auto* insert = sqlite3_mprintf( kInsertSvcEnum.data(),obj.EnumId(), obj.EnumName().c_str(),
                        item.first, item.second.c_str(), obj.Locked() ? 1 : 0 );
        const std::string sql = insert;
        sqlite3_free(insert);
        ExecuteSql(sql);
      }
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to create the SVCENUM table. Error: " << err.what();
    return false;
  }
  return true;
}



bool SqliteDatabase::CreateSvcEntTable(const IModel &model) {
  try {
    ExecuteSql(kCreateSvcEnt.data());
    auto table_list = model.AllTables();

    // Insert tables
    for (const auto* table : table_list) {
      if (table == nullptr) {
        continue;
      }
      auto* insert = sqlite3_mprintf( kInsertSvcEnt.data(),
                                      table->ApplicationId(),
                                      table->ApplicationName().c_str(),
                                      static_cast<int>(table->BaseId()),
                                      table->DatabaseName().empty() ? nullptr : table->DatabaseName().c_str(),
                                      table->SecurityMode(),
                                      table->Description().empty() ? nullptr : table->Description().c_str() );
      const std::string sql = insert;
      sqlite3_free(insert);
      ExecuteSql(sql);
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to create the SVCENT table. Error: " << err.what();
    return false;
  }

  return true;
}

bool SqliteDatabase::CreateSvcAttrTable(const IModel &model) {
  try {
    ExecuteSql(kCreateSvcAttr.data());
    const auto table_list = model.AllTables();

    // Insert enumerates
    for (const auto* table : table_list) {
      if (table == nullptr) {
        continue;
      }
      auto* tab = const_cast<ITable*>(table);
      if (tab == nullptr) {
        continue;
      }
      auto& column_list = tab->Columns();
      for (auto& column : column_list) {
        if (column.ColumnId() <= 0) {
          column.ColumnId(tab->FindNextColumnId());
        }
        if (column.TableId() != tab->ApplicationId()) {
          column.TableId(tab->ApplicationId());
        }
        auto* insert = sqlite3_mprintf( kInsertSvcAttr.data(),
              column.TableId(),
              column.ColumnId(),
              column.ApplicationName().c_str(),
              column.BaseName().empty() ? nullptr : column.BaseName().c_str(),
              column.ReferenceId() > 0 ? std::to_string(column.ReferenceId()).c_str() : "NULL",
              column.UnitIndex() > 0 ? std::to_string(column.UnitIndex()).c_str() : "NULL",
              static_cast<int>(column.DataType()),
              static_cast<int>(column.NofDecimals()),
              column.DatabaseName().c_str(),
              column.AclIndex(),
              column.ReferenceName().empty() ? nullptr : column.ReferenceName().c_str(),
              static_cast<int>(column.Flags()),
              column.EnumName().empty() ? nullptr : column.EnumName().c_str(),
              column.Description().empty() ? nullptr : column.Description().c_str(),
              column.DisplayName().empty() ? nullptr : column.DisplayName().c_str(),
              static_cast<int>(column.NofDecimals()),
              column.DefaultValue().empty() ? nullptr : column.DefaultValue().c_str());
        const std::string sql = insert;
        sqlite3_free(insert);
        ExecuteSql(sql);
      }
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to create the SVCATTR table. Error: " << err.what();
    return false;
  }
  return true;
}

bool SqliteDatabase::CreateTables(const IModel &model) {
  try {

    const auto table_list = model.AllTables();
    for (const auto* table : table_list) {
      if (table == nullptr) {
        continue;
      }
      if (table->DatabaseName().empty() || table->Columns().empty()) {
        continue;
      }
      // Create the table
      const auto sql = MakeCreateTableSql(model, *table);
      ExecuteSql(sql);

      // Now create all indexes. This is a bit complicated as if all unique columns have an index, then
      // a compound index is wanted but if only one of them then it is an ordinary index.
      const auto unique_list = table->MakeUniqueList();
      const auto unique_index = std::ranges::all_of(unique_list, [] (const auto& col) { return col.Index(); });
      if (unique_index && !unique_list.empty()) {
        std::ostringstream create_ix;
        create_ix << "CREATE UNIQUE INDEX IF NOT EXISTS IX_" << table->DatabaseName();
        for ( const auto& col1 : unique_list) {
          create_ix << "_" << col1.DatabaseName();
        }
        create_ix << " ON " << table->DatabaseName() << "(";
        for ( size_t col2 = 0; col2 < unique_list.size(); ++col2) {
          if (col2 > 0) {
            create_ix << ",";
          }
          create_ix << unique_list[col2].DatabaseName();
        }
        create_ix << ")";
        ExecuteSql(create_ix.str());
      }

      const auto& column_list = table->Columns();
      for (const auto& column : column_list) {
        // Avoid primary key and no database columns
        if (column.DatabaseName().empty() || IEquals(column.BaseName(), "id") || !column.Index()) {
          continue;
        }
        if (column.Unique() && unique_index) { // Index added above
          continue;
        }
        std::ostringstream create_ix;
        create_ix << "CREATE INDEX IF NOT EXISTS IX_" << table->DatabaseName() << "_" << column.DatabaseName()
            << " ON " << table->DatabaseName() << "(" << column.DatabaseName() << ")";
        ExecuteSql(create_ix.str());
      }
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to create the DB tables. Error: " << err.what();
    return false;
  }

  return true;
}

void SqliteDatabase::Insert(const ITable &table, IItem &row) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  const auto &column_list = table.Columns();
  if (table.DatabaseName().empty() || column_list.empty()) {
    return;
  }

  std::ostringstream insert;
  insert << "INSERT INTO " << table.DatabaseName() << " (";
  bool first1 = true;
  for (const auto &col1: column_list) {
    if (IEquals(col1.BaseName(), "id") || col1.DatabaseName().empty()) {
      continue;
    }
    if (!first1) {
      insert << ",";
    } else {
      first1 = false;
    }
    insert << col1.DatabaseName();
  }
  insert << ") VALUES (";

  bool first2 = true;
  for (const auto &col2: column_list) {
    if (IEquals(col2.BaseName(), "id") || col2.DatabaseName().empty()) {
      continue;
    }
    if (!first2) {
      insert << ",";
    } else {
      first2 = false;
    }
    const auto *item = row.GetAttribute(col2.ApplicationName());
    if (item != nullptr) {
      if (col2.IsString()) {
        const auto val = item->Value<std::string>();
        if (val.empty() && !col2.Obligatory() && col2.DefaultValue().empty()) {
          insert << "NULL";
        } else {
          auto *value = sqlite3_mprintf("%Q", item->Value<std::string>().c_str());
          insert << value;
          sqlite3_free(value);
        }
      } else if (col2.ReferenceId() > 0 && item->Value<int64_t>() <= 0) {
        insert << "NULL";
      } else {
        insert << item->Value<std::string>();
      }
    } else if (!col2.DefaultValue().empty()) {
      if (col2.IsString()) {
        auto *value = sqlite3_mprintf("%Q", col2.DefaultValue().c_str());
        insert << value;
        sqlite3_free(value);
      } else {
        insert << col2.DefaultValue();
      }
    } else if (col2.Obligatory()) {
      if (col2.IsString()) {
        auto *value = sqlite3_mprintf("%Q", "");
        insert << value;
        sqlite3_free(value);
      } else {
        insert << "0";
      }
    } else {
      insert << "NULL";
    }
  }
  insert << ")";
  ExecuteSql(insert.str());
  const auto index = sqlite3_last_insert_rowid(database_);
  row.ItemId(index);
}

void SqliteDatabase::Update(const ITable &table, IItem &row, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }
  const auto &column_list = table.Columns();
  const auto &attr_list = row.AttributeList();
  if (table.DatabaseName().empty() || column_list.empty() || attr_list.empty()) {
    return;
  }

  std::ostringstream update;
  update << "UPDATE " << table.DatabaseName() << " SET ";
  bool first = true;
  for (const auto &attr: attr_list) {
    const auto* column = table.GetColumnByName(attr.Name());
    if (column == nullptr || column->DatabaseName().empty() ||
       IEquals(column->BaseName(), "id")) {
      continue;
    }

    if (!first) {
      update << ",";
    } else {
      first = false;
    }
    update << column->DatabaseName() << "=";
    const std::string val = attr.Value<std::string>();
    if (column->IsString()) {
      if (val.empty() && !column->Obligatory() && column->DefaultValue().empty()) {
        update << "NULL";
      } else {
        auto *value = sqlite3_mprintf("%Q", val.c_str());
        update << value;
        sqlite3_free(value);
      }
    } else {
      update << val;
    }
    update << " " << filter.GetWhereStatement();
  }
  ExecuteSql(update.str());
}

void SqliteDatabase::Delete(const ITable &table, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  if (filter.IsEmpty()) {
    // If filter is empty the entire table is deleted. Treat as error
    throw std::runtime_error("There is no where statement in the delete");
  }

  std::ostringstream del;
  del << "DELETE FROM " << table.DatabaseName() << " " << filter.GetWhereStatement();
  ExecuteSql(del.str());
}

bool SqliteDatabase::InsertModelUnits(const IModel &model) {
  const auto* unit_table = model.GetBaseId(BaseId::AoUnit);
  if (unit_table == nullptr) {
    LOG_DEBUG() << "No unit table in DB. Assume no model units";
    return true;
  }
  const auto* name_column = unit_table->GetColumnByBaseName("name");
  if (name_column == nullptr) {
    LOG_ERROR() << "No name column in the unit table. Table: " << unit_table->DatabaseName();
    return false;
  }

  // Need a temporary list of inserted units
  std::unordered_map<std::string, int64_t> inserted_list;
  const auto table_list = model.AllTables();
  for (const auto* tab : table_list) {
    if (tab == nullptr) {
      continue;
    }
    auto* table = const_cast<ITable*>(tab);
    if (table == nullptr) {
      continue;
    }
    auto& column_list = table->Columns();
    for (auto& column : column_list) {
      if (column.Unit().empty()) {
        continue;
      }

      try {
        const auto exist = inserted_list.find(column.Unit());
        if (exist != inserted_list.cend()) {
          column.UnitIndex(exist->second);
        } else {
          IItem row(unit_table->ApplicationId());
          row.AppendAttribute({name_column->ApplicationName(), column.Unit()});
          Insert(*unit_table, row);
          column.UnitIndex(row.ItemId());
          inserted_list.insert({column.Unit(), column.UnitIndex()});
        }
        std::ostringstream update;
        update << "UPDATE SVCATTR SET FUNIT = " << column.UnitIndex()
               << " WHERE AID = " << table->ApplicationId()
               << " AND ATTRNR = " << column.ColumnId();
        ExecuteSql(update.str());
      } catch (const std::exception& err) {
        LOG_ERROR() << "Failed to insert model units";
        return false;
      }
    }
  }
  return true;
}

bool SqliteDatabase::InsertModelEnvironment(const IModel &model) {
  const auto* env_table = model.GetBaseId(BaseId::AoEnvironment);
  if (env_table == nullptr) {
    LOG_DEBUG() << "No environment table in DB. Assume no model units";
    return true;
  }

  const auto* name_column = env_table->GetColumnByBaseName("name");
  if (name_column == nullptr || env_table->DatabaseName().empty()) {
    LOG_DEBUG() << "No environment table in DB.";
    return true;
  }

  IItem env(env_table->ApplicationId());
  env.AppendAttribute({name_column->ApplicationName(),model.Name()});

  const auto* desc = env_table->GetColumnByBaseName("description");
  if (desc != nullptr) {
    env.AppendAttribute({desc->ApplicationName(),model.Description()});
  }

  const auto* version = env_table->GetColumnByBaseName("version");
  if (version != nullptr) {
    env.AppendAttribute({version->ApplicationName(),model.Version()});
  }

  const auto* created = env_table->GetColumnByBaseName("ao_created");
  if (created != nullptr) {
    env.AppendAttribute({created->ApplicationName(),NsToIsoTime(model.Created(), 0)});
  }

  const auto* created_by = env_table->GetColumnByBaseName("ao_created_by");
  if (created_by != nullptr) {
    env.AppendAttribute({created_by->ApplicationName(),model.CreatedBy()});
  }

  const auto* modified = env_table->GetColumnByBaseName("ao_last_modified");
  if (modified != nullptr) {
    env.AppendAttribute({modified->ApplicationName(),NsToIsoTime(model.Modified(), 0)});
  }

  const auto* modified_by = env_table->GetColumnByBaseName("ao_last_modified_by");
  if (modified_by != nullptr) {
    env.AppendAttribute({modified_by->ApplicationName(),model.ModifiedBy()});
  }

  const auto* base_version = env_table->GetColumnByBaseName("base_model_version");
  if (base_version != nullptr) {
    env.AppendAttribute({base_version->ApplicationName(),model.BaseVersion()});
  }

  try {
    Insert(*env_table, env);
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to insert model environment";
    return false;
  }
  return true;
}

bool SqliteDatabase::ReadSvcEnumTable(IModel &model) {
  try {
    SqliteStatement select(database_, "SELECT * FROM SVCENUM");
    const auto enum_id = select.GetColumnIndex("ENUMID");
    const auto enum_name = select.GetColumnIndex("ENUMNAME");
    const auto item = select.GetColumnIndex("ITEM");
    const auto item_name = select.GetColumnIndex("ITEMNAME");
    const auto locked = select.GetColumnIndex("LOCKED");

    for (bool more = select.Step(); more ; more = select.Step()) {
      const auto ident = select.Value<int64_t>(enum_id);
      const auto name = select.Value<std::string>(enum_name);
      const auto item_index = select.Value<int64_t>(item);
      const auto item_id = select.Value<std::string>(item_name);
      const auto lock = select.Value<bool>(locked);

      if (name.empty()) {
        continue;
      }
      auto* enum_obj = model.GetEnum(name);
      if ( enum_obj == nullptr) {
        IEnum new_enum;
        new_enum.EnumId(ident);
        new_enum.EnumName(name);
        new_enum.Locked(lock);
        new_enum.AddItem(item_index, item_id);
        model.AddEnum(new_enum);
      } else {
        enum_obj->AddItem(item_index, item_id);
      }
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to read the SVCENUM table. Error: " << err.what();
    return false;
  }

  return true;
}
bool SqliteDatabase::ReadSvcEntTable(IModel &model) {
  try {
    SqliteStatement select(database_, "SELECT * FROM SVCENT");
    const auto app_id = select.GetColumnIndex("AID");
    const auto app_name = select.GetColumnIndex("ANAME");
    const auto base_id = select.GetColumnIndex("BID");
    const auto dbt_name = select.GetColumnIndex("DBTNAME");
    const auto security = select.GetColumnIndex("SECURITY");
    const auto desc = select.GetColumnIndex("DESC");
    const auto father_id = select.GetColumnIndex("FAID");

    for (bool more = select.Step(); more ; more = select.Step()) {
      ITable table;
      table.ApplicationId(select.Value<int64_t>(app_id));
      table.ApplicationName(select.Value<std::string>(app_name));
      table.BaseId(static_cast<BaseId>(select.Value<int>(base_id)));
      table.DatabaseName(select.Value<std::string>(dbt_name));
      table.SecurityMode(select.Value<int64_t>(security));
      table.Description(select.Value<std::string>(desc));
      table.ParentId(select.Value<int64_t>(father_id));
      model.AddTable(table); // Note that the list now is temporary with no parent relations
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to read the SVCENT table. Error: " << err.what();
    return false;
  }

  return true;
}

bool SqliteDatabase::ReadSvcAttrTable(IModel &model) {
  try {
    SqliteStatement select(database_, "SELECT * FROM SVCATTR");
    const auto app_id = select.GetColumnIndex("AID");
    const auto attr_id = select.GetColumnIndex("ATTRNR");
    const auto app_name = select.GetColumnIndex("AANAME");
    const auto base_name = select.GetColumnIndex("BANAME");
    const auto father_id = select.GetColumnIndex("FAID");
    const auto unit_id = select.GetColumnIndex("FUNIT");
    const auto data_type = select.GetColumnIndex("ADTYPE");
    const auto data_length = select.GetColumnIndex("AFLEN");
    const auto dbc_name = select.GetColumnIndex("DBCNAME");
    const auto acl_ref = select.GetColumnIndex("ACLREF");
    const auto ref_name = select.GetColumnIndex("INVNAME");
    const auto flag = select.GetColumnIndex("FLAG");
    const auto enum_name = select.GetColumnIndex("ENUMNAME");
    const auto desc = select.GetColumnIndex("DESC");
    const auto display_name = select.GetColumnIndex("DISPNAME");
    const auto decimals = select.GetColumnIndex("NOFDEC");
    const auto def_val = select.GetColumnIndex("DEFVALUE");

    for (bool more = select.Step(); more ; more = select.Step()) {
      IColumn column;
      column.TableId(select.Value<int64_t>(app_id));
      column.ColumnId(select.Value<int64_t>(attr_id));
      column.ReferenceId(select.Value<int64_t>(father_id));
      column.UnitIndex(select.Value<int64_t>(unit_id));

      column.AclIndex(select.Value<int64_t>(acl_ref));
      column.DataType(static_cast<DataType>(select.Value<int>(data_type)));
      column.DataLength(select.Value<size_t>(data_length));
      column.Flags(select.Value<uint16_t>(flag));
      column.NofDecimals(select.Value<int>(decimals));

      column.ApplicationName(select.Value<std::string>(app_name));
      column.BaseName(select.Value<std::string>(base_name));
      column.DatabaseName(select.Value<std::string>(dbc_name));
      column.ReferenceName(select.Value<std::string>(ref_name));
      column.Description(select.Value<std::string>(desc));
      column.DisplayName(select.Value<std::string>(display_name));
      column.EnumName(select.Value<std::string>(enum_name));
      column.DefaultValue(select.Value<std::string>(def_val));

      const auto* tab = model.GetTable(column.TableId());
      auto* table = tab != nullptr ? const_cast<ITable*>(tab) : nullptr;
      if (table != nullptr) {
        const auto parent_list = GetParentBaseName(table->BaseId());
        const auto parent = std::ranges::any_of(parent_list, [&] (const auto& base) {
          return IEquals(base, column.BaseName());
        });
        if (parent) {
          table->ParentId(column.ReferenceId());
        }
        table->AddColumn(column);
      }
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to read the SVCATTR table. Error: " << err.what();
    return false;
  }

  auto temp_list = model.Tables(); // copy the tables
  model.ClearTableList();
  for (size_t count = 0; !temp_list.empty() && count < 100; ++count) {
    for (auto itr = temp_list.begin(); itr != temp_list.cend(); /* No ++itr here */) {
      const auto &table = itr->second;
      const auto *parent = table.ParentId() > 0 ? model.GetTable(table.ParentId()) : nullptr;
      if (table.ParentId() <= 0 || parent != nullptr) {
        model.AddTable(table);
        itr = temp_list.erase(itr);
      } else {
        ++itr;
      }
    }
  }
  return true;
}

bool SqliteDatabase::FixUnitStrings(const IModel &model) {
  try {
    const auto* unit_table = model.GetBaseId(BaseId::AoUnit);
    if (unit_table == nullptr) {
      return true;
    }
    IdNameMap unit_list;
    FetchNameMap(*unit_table, unit_list);
    const auto table_list = model.AllTables();
    for (const auto* tab : table_list) {
      auto* table = tab == nullptr ? nullptr : const_cast<ITable*>(tab);
      if (table == nullptr) {
        continue;
      }
      auto& column_list = table->Columns();
      for (auto& column : column_list) {
        if (column.UnitIndex() > 0) {
          const auto itr = unit_list.find(column.UnitIndex());
          if (itr != unit_list.cend()) {
            column.Unit(itr->second);
          }
        }
      }
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to fix the model units strings.";
    return false;
  }

  return true;
}

void SqliteDatabase::FetchNameMap(const ITable &table, IdNameMap& dest_list, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open.");
  }

  const auto* column_id = table.GetColumnByBaseName("id");
  const auto* column_name = table.GetColumnByBaseName("name");
  if (table.DatabaseName().empty() || column_id == nullptr || column_name == nullptr ) {
    return;
  }

  std::ostringstream sql;
  sql << "SELECT " << column_id->DatabaseName() << "," << column_name->DatabaseName()
      << " FROM " << table.DatabaseName() ;
  if (!filter.IsEmpty()) {
    sql << " " << filter.GetWhereStatement();
  }

  SqliteStatement select(database_, sql.str());
  for (bool more = select.Step(); more ; more = select.Step()) {
    const auto index = select.Value<int64_t>(0);
    const auto name = select.Value<std::string>(1);
    dest_list.insert({index, name});
  }
}

void SqliteDatabase::FetchItemList(const ITable &table, ItemList& dest_list, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open.");
  }


  if (table.DatabaseName().empty()) {
    return;
  }

  std::ostringstream sql;
  sql << "SELECT * FROM " << table.DatabaseName() ;
  if (!filter.IsEmpty()) {
    sql << " " << filter.GetWhereStatement();
  }

  SqliteStatement select(database_, sql.str());
  for (bool more = select.Step(); more ; more = select.Step()) {
    const auto& column_list = table.Columns();
    auto item = std::make_unique<IItem>();
    item->ApplicationId(table.ApplicationId());

    for (const auto& column : column_list) {
      const auto index = select.GetColumnIndex(column.DatabaseName());
      if (index < 0) {
        continue;
      }
      item->AppendAttribute({column.ApplicationName(), column.BaseName(), select.Value<std::string>(index)});
    }
    dest_list.push_back(std::move(item));
  }
}

bool SqliteDatabase::FetchModelEnvironment(IModel &model) {

  try {
      // Pre-fill with file information in case no environment row. This is actually the normal case.
    const std::filesystem::path full_name(FileName());
    const auto last = std::filesystem::last_write_time(full_name);
    const auto sys_time = std::chrono::file_clock::to_sys(last);
    const uint64_t ns1970 = util::time::TimeStampToNs(sys_time);
    model.Modified(ns1970);
    model.Created(ns1970);
    model.Name(full_name.stem().string());
    model.SourceInfo(FileName());

    const auto* env_table = model.GetBaseId(BaseId::AoEnvironment);
    if (env_table == nullptr || env_table->DatabaseName().empty()) {
      return true;
    }

    std::ostringstream sql;
    sql << "SELECT * FROM " << env_table->DatabaseName();
    SqliteStatement select(database_, sql.str());
    for (bool more = select.Step(); more ; more = select.Step()) {
      model.Name(select.Value<std::string>(env_table->GetColumnByBaseName("name")));
      model.Version(select.Value<std::string>(env_table->GetColumnByBaseName("version")));
      model.Description(select.Value<std::string>(env_table->GetColumnByBaseName("description")));
      const auto *version_date = env_table->GetColumnByBaseName("version_date");
      if (version_date != nullptr) {
        model.Created(IsoTimeToNs(select.Value<std::string>(version_date)));
        model.Modified(IsoTimeToNs(select.Value<std::string>(version_date)));
      }

      model.CreatedBy(select.Value<std::string>(env_table->GetColumnByBaseName("ao_created_by")));
      const auto* created = env_table->GetColumnByBaseName("ao_created");
      if (created != nullptr) {
        model.Created(IsoTimeToNs(select.Value<std::string>(created)));
      }

      model.ModifiedBy(select.Value<std::string>(env_table->GetColumnByBaseName("ao_modified_by")));
      const auto* modified = env_table->GetColumnByBaseName("ao_modified");
      if (modified != nullptr) {
        model.Modified(IsoTimeToNs(select.Value<std::string>(modified)));
      }

      model.BaseVersion(select.Value<std::string>(env_table->GetColumnByBaseName("base_model_version")));

      if (model.Version().empty()) {
        model.Version(select.Value<std::string>(env_table->GetColumnByBaseName("application_model_version")));
      }
      model.SourceType(select.Value<std::string>(env_table->GetColumnByBaseName("application_model_type")));
    }

  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to read the environment table. Error: " << err.what();
    return false;
  }
  return true;
}

} // end namespace

