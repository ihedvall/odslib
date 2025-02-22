/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "sqlitedatabase.h"
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <string_view>
#include <algorithm>

#include <util/logstream.h>
#include <util/stringutil.h>
#include <util/timestamp.h>

#include "ods/baseattribute.h"
#include "sqlitestatement.h"
#include "odshelper.h"
using namespace std::chrono_literals;
using namespace util::log;
using namespace util::string;
using namespace util::time;
namespace {

int BusyHandler(void* , int nof_locks) {
  if (nof_locks < 1000) {
    std::this_thread::sleep_for(10ms);
    return 1;
  }
  return 0;
}

void AddAttribute(const ods::IColumn& column,
                  const ods::detail::SqliteStatement& select,
                  int index,
                  ods::IItem& row) {
  using namespace ods;
  if (index < 0) {
    return;
  }
  IAttribute attr(column.ApplicationName(), column.BaseName(), "");
  switch (column.DataType()) {
    case DataType::DtEnum:
    case DataType::DtId:
    case DataType::DtLongLong:
    case DataType::DtLong:
    case DataType::DtByte:
    case DataType::DtShort:attr.Value(select.Value<int64_t>(index));
      break;

    case DataType::DtDouble:
    case DataType::DtFloat:attr.Value(select.Value<double>(index));
      break;

    case DataType::DtBoolean:attr.Value(select.Value<bool>(index));
      break;

    case DataType::DtBlob:
    case DataType::DtByteString:attr.Value(select.Value<std::vector<uint8_t>>(index));
      break;

    case DataType::DtExternalRef:
    case DataType::DtDate: // Stored as string in SQLite
    case DataType::DtString:
    default:attr.Value(select.Value<std::string>(index));
      break;
  }
  row.AppendAttribute(attr);
}

} // end namespace

namespace ods::detail {
SqliteDatabase::SqliteDatabase() {
  DatabaseType(DbType::TypeSqlite);
}
SqliteDatabase::SqliteDatabase(const std::string &filename) {
  DatabaseType(DbType::TypeSqlite);
  FileName(filename);
}

SqliteDatabase::~SqliteDatabase() {
  SqliteDatabase::Close(false);
}

bool SqliteDatabase::Open() {
  bool open = false;
  try {
    if (database_ == nullptr) {
      if (!std::filesystem::exists(FileName()) ) {
        std::ostringstream err;
        err << "Database file does not exist. File: " << FileName();
        throw std::runtime_error(err.str());
      }

      int open3;
      size_t locks = 0;
      for (open3 = sqlite3_open_v2(FileName().c_str(), &database_, SQLITE_OPEN_READWRITE, nullptr);
           open3 == SQLITE_BUSY && locks < 1000;
           open3 = sqlite3_open(FileName().c_str(), &database_) ) {
          std::this_thread::sleep_for(10ms);
          ++locks;
      }

      if (open3 != SQLITE_OK) {
        if (database_ != nullptr) {
          const auto error = sqlite3_errmsg(database_);
          sqlite3_close_v2(database_);
          database_ = nullptr;
          LOG_ERROR() << "Failed to open the database. Error: " << error
                      << ", File: " << FileName();
        } else {
          LOG_ERROR() << "Failed to open the database. File: " << FileName();
        }
      }
    }

    if (database_ != nullptr) {
      sqlite3_busy_handler(database_,BusyHandler, this);
      if (listen_ && listen_->IsActive()) {
        sqlite3_trace_v2(database_, SQLITE_TRACE_STMT | SQLITE_TRACE_PROFILE
                                        | SQLITE_TRACE_ROW | SQLITE_TRACE_CLOSE,
                         TraceCallback, this);
        listen_->ListenTextEx(util::time::TimeStampToNs(), Name(),
                              "Open database.");
      } else {
        sqlite3_trace_v2(database_, 0, nullptr, nullptr);
      }

      if (use_constraints_) {
        ExecuteSql("PRAGMA foreign_keys = ON");
      } else {
        ExecuteSql("PRAGMA foreign_keys = OFF");
      }

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
  const auto open3 = sqlite3_open_v2(FileName().c_str(), &database_, flags,
                                     nullptr);
  if (open3 != SQLITE_OK) {
    if (database_ != nullptr) {
      const auto error = sqlite3_errmsg(database_);
      sqlite3_close_v2(database_);
      database_ = nullptr;
      LOG_ERROR() << "Failed to open the database. Error: " << error
                  << ", File: " << FileName();
    } else {
      LOG_ERROR() << "Failed to open the database. File: " << FileName();
    }
  }
  if (database_ != nullptr) {
    sqlite3_busy_handler(database_,BusyHandler, this);
    if (listen_ && listen_->IsActive()) {
      sqlite3_trace_v2(database_, SQLITE_TRACE_STMT | SQLITE_TRACE_PROFILE
                                      | SQLITE_TRACE_ROW | SQLITE_TRACE_CLOSE,
                       TraceCallback, this);
      listen_->ListenTextEx(util::time::TimeStampToNs(), Name(),
                            "OpenEx database.");
    } else {
      sqlite3_trace_v2(database_, 0, nullptr, nullptr);
    }
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
    LOG_ERROR() << "Failed to close the database. Error: "
                << sqlite3_errmsg(database_) << ", File: " << FileName();
  }
  database_ = nullptr;
  return close == SQLITE_OK;
}

bool SqliteDatabase::IsOpen() const {
  return database_ != nullptr;
}

bool SqliteDatabase::ExistDatabaseTable(const std::string &dbt_name)  {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open.");
  }
  if (dbt_name.empty()) {
    return false;
  }

  std::ostringstream sql;
  sql << "SELECT COUNT(*)  FROM sqlite_master "
      << "WHERE type='table' AND 'name=" << dbt_name << "'" ;
  const auto ret_val = ExecuteSql(sql.str());
  return ret_val > 0;
}

int SqliteDatabase::ExecCallback(void *object, int rows, char **value_list,
                                 char **column_list) {
  auto* database = reinterpret_cast<SqliteDatabase*>(object);
  if (database == nullptr) {
    return 1;
  }
  for (int row = 0; row < rows; ++row) {
    if (value_list == nullptr || column_list == nullptr) {
      return 0;
    }
    const auto* value = value_list[row];
    if (value == nullptr) {
      continue;
    }
    try {
      auto temp = std::stoll(value);
      database->exec_result_ += temp;
    } catch (const std::exception&) {
    }
  }
  return 0;
}

int64_t SqliteDatabase::ExecuteSql(const std::string &sql) {
  if (database_ == nullptr) {
    throw std::runtime_error("Database not open");
  }
  exec_result_ = 0; // Variable updated by the callback.
  char* error = nullptr;
  sqlite3_exec(database_, sql.c_str(), ExecCallback, this,
                                 &error);
  if (error != nullptr) {
    std::ostringstream err;
    err << "SQL Execute error. Error: " << error << ", SQL:" << sql;
    sqlite3_free(error);
    LOG_ERROR() << err.str();
    throw std::runtime_error(err.str());
  }
  return exec_result_;
}

sqlite3 *SqliteDatabase::Sqlite3() {
  return database_;
}


void SqliteDatabase::ConnectionInfo(const std::string &info) {
  IDatabase::ConnectionInfo(info); // May be changed by the filename function
  FileName(info);
}

void SqliteDatabase::FileName(const std::string &filename) {
  // It seems that the SQLITE is sensible about the slashes
  try {
    std::filesystem::path file(filename);
    file.make_preferred();
    IDatabase::ConnectionInfo(file.string());
    if (Name().empty()) {
      Name(file.stem().string());
    }
  } catch (const std::exception& error) {
    LOG_ERROR() << "File name path error. Error: " << error.what()
      << ", File Name: " << filename;
  }
}

bool SqliteDatabase::Create(const IModel &model) {
  const auto open = OpenEx(); // Special open that creates the file
  if (!open) {
    LOG_ERROR() << "Failed to create an empty SQLITE database. DB: "
                << FileName();
    return false;
  }
  const auto svc_enum = CreateSvcEnumTable(model);
  const auto svc_ent = CreateSvcEntTable(model);
  const auto svc_attr = CreateSvcAttrTable(model);
  const auto svc_ref = CreateSvcRefTable(model);

  const auto tables = CreateTables(model);
  const auto relation_tables = CreateRelationTables(model);
  const auto units = InsertModelUnits(model);
  const auto env = InsertModelEnvironment(model);

  const auto close = Close(true);
  return close && svc_enum && svc_ent && svc_attr && svc_ref && tables
               && relation_tables && units && env;
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


bool SqliteDatabase::ReadSvcRefTable(IModel &model) {
  model.GetRelationList().clear();
  if (!ExistDatabaseTable("SVCREF")) {
    return true;
  }
  try {
    SqliteStatement select(database_, "SELECT * FROM SVCREF");
    const auto app_id1 = select.GetColumnIndex("AID1");
    const auto app_id2 = select.GetColumnIndex("AID2");
    const auto ref_name = select.GetColumnIndex("REFNAME");
    const auto dbt_name = select.GetColumnIndex("DBTNAME");
    const auto inv_name = select.GetColumnIndex("INVNAME");
    const auto base_name = select.GetColumnIndex("BANAME");
    const auto inv_base_name = select.GetColumnIndex("INVBANAME");

    for (bool more = select.Step(); more ; more = select.Step()) {
      IRelation relation;
      relation.ApplicationId1(select.Value<int64_t>(app_id1));
      relation.ApplicationId2(select.Value<int64_t>(app_id2));
      relation.Name(select.Value<std::string>(ref_name));
      relation.DatabaseName(select.Value<std::string>(dbt_name));
      relation.BaseName(select.Value<std::string>(base_name));
      relation.InverseName(select.Value<std::string>(inv_name));
      relation.BaseName(select.Value<std::string>(base_name));
      relation.InverseBaseName(select.Value<std::string>(inv_base_name));
      model.AddRelation(relation);
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to read the SVCREF table. Error: " << err.what();
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
    auto row = std::make_unique<IItem>();
    if (!row) {
      throw std::runtime_error("Failed to allocate a row item.");
    }
    row->ApplicationId(table.ApplicationId());

    for (const IColumn& column : column_list) {
      if (column.DatabaseName().empty()) {
        continue;
      }
      const int index = select.GetColumnIndex(column.DatabaseName());
      AddAttribute(column, select, index, *row);
    }
    dest_list.push_back(std::move(row));
  }
}

size_t SqliteDatabase::FetchItems(const ITable &table, const SqlFilter &filter,
                                std::function<void(IItem&)> OnItem) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open.");
  }
  size_t count = 0;
  if (table.DatabaseName().empty()) {
    return count;
  }

  std::ostringstream sql;
  sql << "SELECT * FROM " << table.DatabaseName() ;
  if (!filter.IsEmpty()) {
    sql << " " << filter.GetWhereStatement();
  }

  SqliteStatement select(database_, sql.str());
  for (bool more = select.Step(); more ; more = select.Step()) {
    const auto& column_list = table.Columns();

    IItem row;
    row.ApplicationId(table.ApplicationId());
    for (const IColumn& column : column_list) {
      int index = select.GetColumnIndex(column.DatabaseName());
      if (index < 0) {
        continue;
      }
      AddAttribute(column, select, index, row);
    }
    OnItem(row);
    ++count;
  }
  return count;
}

bool SqliteDatabase::FetchModelEnvironment(IModel &model) {

  try {
      // Pre-fill with file information in case no environment row. This is actually the normal case.
    const std::filesystem::path full_name(FileName());
    const auto last = std::filesystem::last_write_time(full_name);
    const auto ns1970 = util::time::FileTimeToNs(last);
    model.Modified(ns1970);
    model.Created(ns1970);
    model.Name(full_name.stem().string());
    model.SourceInfo(FileName());

    const auto* env_table = model.GetTableByBaseId(BaseId::AoEnvironment);
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

int SqliteDatabase::TraceCallback(unsigned int mask, void *context, void *arg1, void *arg2) {
  if (context == nullptr) {
    return 0;
  }
  auto* database = reinterpret_cast<SqliteDatabase*> (context);
  if (!database->listen_ || !database->listen_->IsActive()) {
    return 0;
  }

  switch (mask) {
    case SQLITE_TRACE_STMT: {
      if (arg2 == nullptr) {
        return 0;
      }
      const auto* sql = reinterpret_cast<const char*>(arg2);
      if (database->listen_->LogLevel() == 1) {
        // Show statement and rows
        database->listen_->ListenTextEx(util::time::TimeStampToNs(),
                                        database->Name(),
                                        "%s", sql);
      }
      database->row_count_ = 0;
      break;
    }

    case SQLITE_TRACE_PROFILE: {
      if (arg1 == nullptr || arg2 == nullptr) {
        return 0;
      }
      auto* stmt = reinterpret_cast<sqlite3_stmt*>(arg1);
      const auto* nano_sec = reinterpret_cast<int64_t*>(arg2);
      const auto* sql = sqlite3_sql(stmt);
      if (sql != nullptr && database->listen_->LogLevel() == 0) {
          // Show statement only
          database->listen_->ListenTextEx(util::time::TimeStampToNs(),
                 database->Name(),"%s (Rows: %d %g ms)", sql,
                 static_cast<int>(database->row_count_),
                 static_cast<double>(*nano_sec) / 1000000.0 );
      }
      break;
    }

    case SQLITE_TRACE_ROW: {
      if (arg1 == nullptr) {
        return 0;
      }

      auto* stmt = reinterpret_cast<sqlite3_stmt*>(arg1);
      const auto* sql = sqlite3_sql(stmt);
      if (sql == nullptr) {
        return 0;
      }
      if (database->listen_->LogLevel() == 1) {
        std::ostringstream out;
        out << "Row " << database->row_count_ << "; ";
        const auto count = sqlite3_data_count(stmt);
        for (int column = 0; column < count; ++column) {
          if (column > 0) {
            out << ",";
          }
          const auto* text = sqlite3_column_text(stmt,column);
          out << (text != nullptr ? reinterpret_cast<const char*>(text) : "NULL");
        }

        // Show data in rows
        database->listen_->ListenTextEx(util::time::TimeStampToNs(),
                                        database->Name(),"%d : %s", static_cast<int>(database->row_count_),
                                        out.str().c_str());
      }
      ++database->row_count_;
      break;
    }

    case SQLITE_TRACE_CLOSE: {
      if (arg1 == nullptr) {
        return 0;
      }
      database->listen_->ListenTextEx(util::time::TimeStampToNs(), database->Name(),"Close database");
      break;
    }

    default:
      break;
  }
  return 0;
}

void SqliteDatabase::Vacuum() {
  if (database_ != nullptr) {
    throw std::runtime_error("The database was open when vacuum the database. Invalid use of function.");
  }

  // Special handling of open and close of the database as the function have some requirements on that
  // functionality.
  const auto open = sqlite3_open_v2(FileName().c_str(), &database_, SQLITE_OPEN_READWRITE, nullptr);
  if (open != SQLITE_OK || database_ == nullptr) {
    std::ostringstream err;
    if (database_ != nullptr) {
      const auto error = sqlite3_errmsg(database_);
      sqlite3_close_v2(database_);
      database_ = nullptr;
      err << "Failed to open the database. Error: " << error << ", File: "
          << FileName();
    } else {
      err << "Failed to open the database. File: " << FileName();
    }
    throw std::runtime_error(err.str());
  }

  try {
    ExecuteSql("VACUUM");
  } catch (const std::exception& err) {
    sqlite3_close_v2(database_);
    database_ = nullptr;
    throw err;
  }
  sqlite3_close_v2(database_);
  database_ = nullptr;
}

std::string SqliteDatabase::DataTypeToDbString(DataType type) {
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

bool SqliteDatabase::IsDataTypeString(DataType type) {
  return DataTypeToDbString(type) == "TEXT";
}

const std::string &SqliteDatabase::FileName() const {
  return IDatabase::ConnectionInfo();
}

void SqliteDatabase::InsertDumpRow(const ITable &table, IItem &row) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  const std::vector<IColumn> &column_list = table.Columns();
  if (table.DatabaseName().empty() || column_list.empty() ) {
    return;
  }


  size_t parameter_count = 1; // Bind index of values
  std::ostringstream insert;
  std::ostringstream values;
  insert << "INSERT INTO " << table.DatabaseName() << " (";
  bool first = true;
  for (const auto &col1: column_list) {
    if (col1.DatabaseName().empty()) {
      continue;
    }
    if (!first) {
      insert << ",";
      values << ",";
    } else {
      first = false;
    }

    insert << col1.DatabaseName();
    values << "?" << parameter_count;
    ++parameter_count;
  }
  insert << ") VALUES (" << values.str() << ")";

  SqliteStatement statement(Sqlite3(),insert.str());
  int value_count = 1;
  for (const auto &col2: column_list) {
    if (col2.DatabaseName().empty()) {
      continue;
    }
    const auto *attr = row.GetAttribute(col2.ApplicationName());
    if (attr != nullptr) {
      // The user has set the item
      switch (col2.DataType()) {

        case DataType::DtShort:
        case DataType::DtByte:
        case DataType::DtLong:
        case DataType::DtLongLong:
        case DataType::DtId:
        case DataType::DtEnum:
          if (col2.ReferenceId() > 0 && attr->Value<int64_t>() <= 0) {
            statement.SetValue(value_count, "NULL");
          } else {
            statement.SetValue(value_count, attr->Value<int64_t>());
          }
          break;

        case DataType::DtFloat:
        case DataType::DtDouble:
          statement.SetValue(value_count, attr->Value<double>());
          break;

        case DataType::DtBoolean:
          statement.SetValue(value_count, attr->Value<bool>());
          break;

        case DataType::DtDate:
          statement.SetValue(value_count, MakeDateValue(*attr)); // Note that this function normally is database-dependent.
          break;

        case DataType::DtString:
        case DataType::DtExternalRef: {
          const std::string val = attr->Value<std::string>();
          if (val.empty() && !col2.Obligatory() && col2.DefaultValue().empty()) {
            statement.SetValue(value_count, "NULL");
          } else {
            char* value = sqlite3_mprintf("%Q", val.c_str());
            statement.SetValue(value_count, value != nullptr ? value : "NULL");
            sqlite3_free(value);
          }
          break;
        }
        case DataType::DtByteString:
        case DataType::DtBlob: {// Convert the Base4 string to a hexadecimal string (X'hexbytes')
          const std::string base64 = attr->Value<std::string>();
          if (base64.empty()) {
            statement.SetValue(value_count, "NULL");
          } else {
            const std::vector<uint8_t> byte_array = OdsHelper::FromBase64(base64);
            statement.SetValue(value_count, byte_array);
          }
          break;
        }

        default:
          if (col2.ReferenceId() > 0 && attr->Value<int64_t>() <= 0) {
            statement.SetValue(value_count, "NULL");
          } else {
            const std::string value = attr->Value<std::string>();
            statement.SetValue(value_count,!value.empty() ? value : "NULL");
          }
          break;
      }
    } else {
      // The user has not set item. First check the DtDate columns
      if (IEquals(col2.BaseName(), "ao_created") ||
          IEquals(col2.BaseName(), "version_date") ||
          IEquals(col2.BaseName(), "ao_last_modified")) {
        // If these columns aren't set, then set them to 'now'.
        // Normal are they set to auto generate.
        // Note that 'ao_last_modified' is set to null at insert and set at update.
        const uint64_t now = TimeStampToNs();
        const std::string timestamp = NsToIsoTime(now, 0);
        std::ostringstream time;
        time << "'" << timestamp << "'";
        statement.SetValue(value_count, time.str()) ;
      } else if (!col2.DefaultValue().empty()) {
        if (col2.IsString()) {
          auto *value = sqlite3_mprintf("%Q", col2.DefaultValue().c_str());
          statement.SetValue(value_count, value);
          sqlite3_free(value);
        } else {
          statement.SetValue(value_count, col2.DefaultValue());
        }
      } else if (col2.Obligatory()) {
        if (col2.IsString()) {
          auto *value = sqlite3_mprintf("%Q", "");
          statement.SetValue(value_count,value);
          sqlite3_free(value);
        } else {
          statement.SetValue(value_count,"0");
        }
      } else {
        statement.SetValue(value_count,"NULL");
      }
    }
    ++value_count;
  }
  statement.Step();
}

void SqliteDatabase::Insert(const ITable &table, IItem &row, const SqlFilter& filter) {

  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }


  const auto &column_list = table.Columns();
  // Note that all table should have an index (id) column but some
  // doesn't so make fixes in case the column is missing.
  const auto* id_column = table.GetColumnByBaseName("id");
  if (table.DatabaseName().empty() || column_list.empty()) {
    return;
  }

  int column_count = 1;
  std::ostringstream insert;
  std::ostringstream values;
  insert << "INSERT INTO " << table.DatabaseName() << " (";
  bool first = true;
  for (const auto &col1: column_list) {
    if (IEquals(col1.BaseName(), "id") || col1.DatabaseName().empty()) {
      continue;
    }
    if (!first) {
      insert << ",";
      values << ",";
    } else {
      first = false;
    }
    insert << col1.DatabaseName();
    values << "?" << column_count;
    ++column_count;
  }
  insert << ") VALUES (" << values.str() << ")";
  if (id_column != nullptr) {
    insert << " RETURNING " << id_column->DatabaseName();
  }

  // Bind all columns except any id column
  SqliteStatement statement(Sqlite3(), insert.str());
  int value_count = 1;
  for (const auto &col2: column_list) {
    if (IEquals(col2.BaseName(), "id") || col2.DatabaseName().empty()) {
      continue;
    }
    const auto *attr = row.GetAttribute(col2.ApplicationName());
    if (attr != nullptr) {
      // The user has set the item
      switch (col2.DataType()) {
        case DataType::DtShort:
        case DataType::DtByte:
        case DataType::DtLong:
        case DataType::DtLongLong:
        case DataType::DtId:
        case DataType::DtEnum:
          if (col2.ReferenceId() > 0 && attr->Value<int64_t>() <= 0) {
            statement.SetValue(value_count, "NULL");
          } else {
            statement.SetValue(value_count, attr->Value<int64_t>());
          }
          break;

        case DataType::DtFloat:
        case DataType::DtDouble:statement.SetValue(value_count, attr->Value<double>());
          break;

        case DataType::DtBoolean:statement.SetValue(value_count, attr->Value<bool>());
          break;

        case DataType::DtDate:
          statement.SetValue(value_count,
                             MakeDateValue(*attr)); // Note that this function normally is database-dependent.
          break;

        case DataType::DtString:
        case DataType::DtExternalRef: {
          const std::string val = attr->Value<std::string>();
          if (val.empty() && !col2.Obligatory() && col2.DefaultValue().empty()) {
            statement.SetValue(value_count, "NULL");
          } else {
            char *value = sqlite3_mprintf("%Q", val.c_str());
            statement.SetValue(value_count, value != nullptr ? value : "NULL");
            sqlite3_free(value);
          }
          break;
        }
        case DataType::DtByteString:
        case DataType::DtBlob: {// Convert the Base4 string to a hexadecimal string (X'hexbytes')
          const std::string base64 = attr->Value<std::string>();
          if (base64.empty()) {
            statement.SetValue(value_count, "NULL");
          } else {
            const std::vector<uint8_t> byte_array = OdsHelper::FromBase64(base64);
            statement.SetValue(value_count, byte_array);
          }
          break;
        }

        default:
          if (col2.ReferenceId() > 0 && attr->Value<int64_t>() <= 0) {
            statement.SetValue(value_count, "NULL");
          } else {
            const std::string value = attr->Value<std::string>();
            statement.SetValue(value_count, !value.empty() ? value : "NULL");
          }
          break;

      }
    } else {
      // The user has not set item. First check the DtDate columns
      if (IEquals(col2.BaseName(), "ao_created") ||
          IEquals(col2.BaseName(), "version_date") ||
          IEquals(col2.BaseName(), "ao_last_modified")) {
        // If these columns aren't set, then set them to 'now'.
        // Normal are they set to auto generate.
        // Note that 'ao_last_modified' is set to null at insert and set at update.
        const uint64_t now = TimeStampToNs();
        const std::string timestamp = NsToIsoTime(now, 0);
        std::ostringstream time;
        time << "'" << timestamp << "'";
        statement.SetValue(value_count, time.str());
      } else if (!col2.DefaultValue().empty()) {
        if (col2.IsString()) {
          auto *value = sqlite3_mprintf("%Q", col2.DefaultValue().c_str());
          statement.SetValue(value_count, value);
          sqlite3_free(value);
        } else {
          statement.SetValue(value_count, col2.DefaultValue());
        }
      } else if (col2.Obligatory()) {
        if (col2.IsString()) {
          auto *value = sqlite3_mprintf("%Q", "");
          statement.SetValue(value_count, value);
          sqlite3_free(value);
        } else {
          statement.SetValue(value_count, 0LL);
        }
      } else {
        statement.SetValue(value_count, "NULL");
      }
    }
    ++value_count;
  }
  statement.Step();
  const auto idx = statement.Value<int64_t>(0);
  row.ItemId(idx);
}


void SqliteDatabase::Update(const ITable &table, IItem &row, const SqlFilter& filter) {

  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  const auto &column_list = table.Columns();
  if (table.DatabaseName().empty() || column_list.empty()) {
    return;
  }

  int column_count = 1;
  std::ostringstream update;
  update << "UPDATE " << table.DatabaseName() << " SET ";
  bool first = true;
  for (const auto &col1: column_list) {
    if (col1.DatabaseName().empty() || IEquals(col1.BaseName(), "id")) {
      continue;
    }

    const bool update_last_changed = IEquals(col1.BaseName(), "ao_last_modified") ||
      IEquals(col1.BaseName(), "version_date");
    const auto *attr = row.GetAttribute(col1.ApplicationName());
    if (attr == nullptr && !update_last_changed) {
      // Do not change current value
      continue;
    }

    if (!first) {
      update << ",";
    } else {
      first = false;
    }
    update << col1.DatabaseName() << "=?" << column_count; // Bind value
    ++column_count;
  }
  update << " " << filter.GetWhereStatement();

  // Bind input values.
  const uint64_t now = TimeStampToNs();
  int value_count = 1;
  SqliteStatement statement(Sqlite3(), update.str());
  for (const auto &col2: column_list) {
    if (col2.DatabaseName().empty() || IEquals(col2.BaseName(), "id")) {
      continue;
    }
    const bool update_last_changed = IEquals(col2.BaseName(), "ao_last_modified") ||
        IEquals(col2.BaseName(), "version_date");
    const auto *attr = row.GetAttribute(col2.ApplicationName());
    if (attr == nullptr && !update_last_changed) {
      // Do not change current value
      continue;
    }

    if (update_last_changed && attr == nullptr) {
      const std::string time_string = NsToIsoTime(now, 0); // Ignore milliseconds
      auto *value = sqlite3_mprintf("%Q", time_string.c_str());
      statement.SetValue(value_count, value);
      sqlite3_free(value);
      ++value_count;
      continue;
    }

    if (attr == nullptr || (attr->IsValueEmpty() && !col2.Obligatory()) ) {
      statement.SetValue(value_count, "NULL");
      ++value_count;
      continue;
    }

    switch (col2.DataType()) {
      case DataType::DtByte:
      case DataType::DtLong:
      case DataType::DtLongLong:
      case DataType::DtId:
      case DataType::DtEnum:
      case DataType::DtShort: {
        const auto value = attr->Value<int64_t>();
        statement.SetValue(value_count, value);
        break;
      }

      case DataType::DtDouble:
      case DataType::DtFloat: {
        const auto value = attr->Value<double>();
        statement.SetValue(value_count, value);
        break;
      }

      case DataType::DtBoolean: {
        const auto value = attr->Value<bool>();
        statement.SetValue(value_count, value ? 1LL : 0LL);
        break;
      }

      case DataType::DtDate: {
        const auto time_string = attr->Value<std::string>();
        auto *value = sqlite3_mprintf("%Q", time_string.c_str());
        statement.SetValue(value_count, value);
        sqlite3_free(value);
        break;
      }

      case DataType::DtBlob:
      case DataType::DtByteString: {
        const auto base64 = attr->Value<std::string>();
        statement.SetValue(value_count, OdsHelper::FromBase64(base64));
        break;
      }

      case DataType::DtExternalRef:
      case DataType::DtString:
      default: {// Default treat as string
        const std::string val = attr->Value<std::string>();
        auto *value = sqlite3_mprintf("%Q", val.c_str());
        statement.SetValue(value_count, value);
        sqlite3_free(value);
        break;
      }
    } // end switch
    ++value_count;
  }

  statement.Step();
}

} // end namespace ods

