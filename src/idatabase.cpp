/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "ods/idatabase.h"
#include <locale>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <charconv>
#include <sqlite3.h>
#include <util/csvwriter.h>
#include <util/logstream.h>
#include <util/stringutil.h>
#include <util/timestamp.h>

#include "ods/databaseguard.h"
#include "odshelper.h"
using namespace util::log;
using namespace util::string;
using namespace util::time;
using namespace std::filesystem;
using namespace std::chrono;

namespace {

constexpr std::string_view kCreateSvcEnum =
    "CREATE TABLE IF NOT EXISTS SVCENUM ("
    "ENUMID  integer NOT NULL, "
    "ENUMNAME varchar NOT NULL, "
    "ITEM integer NOT NULL, "
    "ITEMNAME varchar, "
    "LOCKED integer CHECK (LOCKED IN (0,1)) DEFAULT 1, "
    "CONSTRAINT pk_svcenum PRIMARY KEY (ENUMID,ITEM) )";

constexpr std::string_view kInsertSvcEnum =
    "INSERT INTO SVCENUM ("
    "ENUMID,ENUMNAME ,ITEM, ITEMNAME, LOCKED) VALUES (%lld,%Q, %lld,%Q, %d)";


constexpr std::string_view kCreateSvcEnt =
    "CREATE TABLE IF NOT EXISTS SVCENT ("
   "AID integer PRIMARY KEY NOT NULL, "
   "ANAME varchar NOT NULL UNIQUE, "
   "BID integer NOT NULL, "
   "DBTNAME varchar, "
   "SECURITY integer DEFAULT 0, "
   "\"DESC\" varchar)";

constexpr std::string_view kInsertSvcEnt =
    "INSERT INTO SVCENT ("
    "AID, ANAME ,BID ,DBTNAME, SECURITY, \"DESC\") "
    "VALUES (%lld, %Q, %d, %Q, %lld, %Q)";

constexpr std::string_view kCreateSvcAttr =
    "CREATE TABLE IF NOT EXISTS SVCATTR ("
    "AID integer NOT NULL, "
    "ATTRNR integer, "
    "AANAME varchar NOT NULL, "
    "BANAME varchar, "
    "FAID integer, "
    "FUNIT integer, "
    "ADTYPE integer, "
    "AFLEN integer, "
    "DBCNAME varchar, "
    "ACLREF integer DEFAULT 0, "
    "INVNAME varchar, "
    "FLAG integer, "
    "ENUMNAME varchar, "
    "\"DESC\" varchar, "
    "DISPNAME varchar, "
    "NOFDEC integer, "
    "DEFVALUE varchar, "
    "CONSTRAINT pk_svcattr PRIMARY KEY (AID,AANAME))";

constexpr std::string_view kInsertSvcAttr =
    "INSERT INTO SVCATTR ("
    "AID,ATTRNR,AANAME,BANAME,FAID,FUNIT,ADTYPE,AFLEN,DBCNAME,ACLREF,"
    "INVNAME,FLAG,ENUMNAME,\"DESC\",DISPNAME,NOFDEC,DEFVALUE) "
    "VALUES (%lld, %lld, %Q, %Q, %s, %s, %d, %d, %Q, %lld, %Q, %d, %Q, %Q, "
    "%Q, %d, %Q)";

constexpr std::string_view kCreateSvcRef =
    "CREATE TABLE IF NOT EXISTS SVCREF ("
    "AID1 integer NOT NULL, "
    "AID2 integer NOT NULL, "
    "REFNAME varchar NOT NULL, "
    "DBTNAME varchar NOT NULL, "
    "INVNAME varchar, "
    "BANAME varchar, "
    "INVBANAME varchar, "
    "CONSTRAINT pk_svcref PRIMARY KEY (AID1, AID2, REFNAME))";



constexpr std::string_view kInsertSvcRef =
    "INSERT INTO SVCREF ("
    "AID1,AID2,REFNAME,DBTNAME,INVNAME,BANAME,INVBANAME) "
    "VALUES (%lld, %lld, %Q, %Q, %Q, %Q, %Q)";

std::string GetLocalTimeDirText() {
  const time_t now = std::time(nullptr);
  const struct tm* bt = localtime(&now);
  std::ostringstream text;
  text << std::put_time(bt, "%Y-%m-%d_%H.%M.%S");
  return text.str();
}

} // end namespace

namespace ods {

void IDatabase::Delete(const ITable &table, const SqlFilter& filter) {
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

bool IDatabase::InsertModelUnits(const IModel &model) {
  const auto* unit_table = model.GetTableByBaseId(BaseId::AoUnit);
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
          Insert(*unit_table, row, SqlFilter());
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

bool IDatabase::FixUnitStrings(const IModel &model) {
  try {
    const auto* unit_table = model.GetTableByBaseId(BaseId::AoUnit);
    if (unit_table == nullptr) {
      return true;
    }
    IdNameMap unit_list;
    FetchNameMap(*unit_table, unit_list,SqlFilter());
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

bool IsSqlReservedWord(const std::string &word) {
  return sqlite3_keyword_check(word.c_str(), static_cast<int>(word.size())) > 0;
}

std::string MakeBlobString(const std::vector<uint8_t> &blob) {
  if (blob.empty()) {
    return "NULL";
  }
  std::ostringstream temp;
  temp << "'\\x";

  for (const auto data : blob) {
    temp << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
         << static_cast<int>(data);
  }
  temp << "'";
  return temp.str();
}

std::string IDatabase::MakeDateValue(const IAttribute &attr) const {
  std::string temp;
  if (attr.IsValueUnsigned()) {
    const auto ns1970 = attr.Value<uint64_t>();
    int format = 0;
    if (ns1970 % 1'000'000'000 != 0) {
      format = 1;
    }
    temp = util::time::NsToIsoTime(ns1970,format);
  } else if (attr.IsValueEmpty()) {
    return "NULL";
  } else {
    temp = attr.Value<std::string>();
    if (IEquals(temp,"CURRENT_", 8)) {
      const auto now = TimeStampToNs();
      temp = NsToIsoTime(now, 1);
    }
  }
  std::ostringstream temp_dotted;
  temp_dotted << "'" << temp << "'";
  return temp_dotted.str();
}


void IDatabase::Vacuum() {
  // By default, this function doesn't do anything.
}

void IDatabase::ExportCsv(const std::string& filename, const ITable &table, const SqlFilter &filter) {
  // This method should only be used for tables with a small number of rows.
  util::plot::CsvWriter csv_file(filename);
  const auto& column_list = table.Columns();
  for (const auto& column : column_list) {
    csv_file.AddColumnHeader(column.ApplicationName(),column.Unit(), true);
  }
  ItemList data_list;
  FetchItemList(table, data_list, filter);
  for (const auto& row : data_list) {
    if (!row) {
      continue;
    }
    for (const auto& column : column_list) {
      const auto* attr = row->GetAttribute(column.ApplicationName());
      if (attr == nullptr) {
        csv_file.AddColumnValue(std::string());
        continue;
      }
      switch (column.DataType()) {
        case DataType::DtString:
        case DataType::DtExternalRef:
          csv_file.AddColumnValue(attr->Value<std::string>());
          break;

        case DataType::DtShort:
        case DataType::DtByte:
        case DataType::DtLong:
        case DataType::DtLongLong:
        case DataType::DtId:
        case DataType::DtEnum:
          csv_file.AddColumnValue(attr->Value<int64_t>());
          break;

        case DataType::DtFloat:
          csv_file.AddColumnValue(attr->Value<float>());
          break;

        case DataType::DtDouble:
          csv_file.AddColumnValue(attr->Value<double>());
          break;

        case DataType::DtBoolean:
          csv_file.AddColumnValue(attr->Value<bool>());
          break;

        case DataType::DtDate:
          csv_file.AddColumnValue(attr->Value<uint64_t>());
          break;

        default:
          csv_file.AddColumnValue(std::string());
          break;
      }
    }
    csv_file.AddRow();
  }
  csv_file.CloseFile();
}

bool IDatabase::Create(const IModel &model) {
  const auto open = Open(); // Special open that creates the file
  if (!open) {
    LOG_ERROR() << "Failed to create an empty database. DB: " << Name();
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
  return close && svc_enum && svc_ent && svc_attr && svc_ref
              && tables && relation_tables && units && env;
}

bool IDatabase::CreateSvcEnumTable(const IModel &model) {
  IModel temp_model(model);
  try {
    ExecuteSql(kCreateSvcEnum.data());
    auto& enum_list = temp_model.Enums();

    // Insert enumerates
    for (auto& itr : enum_list) {
      auto& obj = itr.second;
      if (obj.EnumId() <= 0) {
          obj.EnumId(temp_model.FindNextEnumId());
      }
      if (obj.Items().empty()) {
          obj.AddItem(0,"");
      }
      for ( const auto& item : obj.Items()) {
          auto* insert = sqlite3_mprintf( kInsertSvcEnum.data(),obj.EnumId(),
                                         obj.EnumName().c_str(),
                                         item.first, item.second.c_str(),
                                         obj.Locked() ? 1 : 0 );
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

bool IDatabase::CreateSvcEntTable(const IModel &model) {
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

bool IDatabase::CreateSvcAttrTable(const IModel &model) {

  try {
    ExecuteSql(kCreateSvcAttr.data());
    auto table_list = model.AllTables();

    // Insert enumerates
    for (auto* table : table_list) {
      if (table == nullptr) {
          continue;
      }
      auto* tab = const_cast<ITable*>(table);
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

bool IDatabase::CreateSvcRefTable(const IModel &model) {

  try {
    ExecuteSql(kCreateSvcRef.data());
    const auto& relation_list = model.GetRelationList();
    // Insert relation list items
    for (const auto& [name, relation] : relation_list) {
      if (relation.Name().empty() || relation.ApplicationId1() <= 0 || relation.ApplicationId2() <= 0) {
        LOG_ERROR() << "Invalid relation table (SVCREF) found. Name: " << relation.Name()
          << ", AID1: " << relation.ApplicationId1()
          << ", AID2: " << relation.ApplicationId2();
        continue;
      }
      auto* insert = sqlite3_mprintf( kInsertSvcRef.data(),
                            relation.ApplicationId1(),
                            relation.ApplicationId2(),
                            relation.Name().c_str(),
                            relation.DatabaseName().empty() ? nullptr : relation.DatabaseName().c_str(),
                            relation.InverseName().empty() ? nullptr : relation.InverseName().c_str(),
                            relation.BaseName().empty() ? nullptr : relation.BaseName().c_str(),
                            relation.InverseBaseName().empty() ? nullptr : relation.InverseBaseName().c_str());

      const std::string sql = insert;
      sqlite3_free(insert);
      ExecuteSql(sql);
    }
  } catch (std::exception& err) {
    LOG_ERROR() << "Failed to create the SVCREF table. Error: " << err.what();
    return false;
  }
  return true;
}

bool IDatabase::CreateTables(const IModel &model) {
  // Some database support comments on table and column.
  bool use_comment = false; // Add comments to the database
  bool use_serial = false; // Use serial instead of auto increment

  switch (DatabaseType()) {
  case DbType::TypePostgres:
    use_comment = true;
    use_serial = true;
    break;

  case DbType::TypeOracle:
  case DbType::TypeSqlServer:
    use_comment = true;
    break;

  default:
    break;
  }

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
      if (use_comment) {
        AddComments(*table);
      }
      // Now create all indexes.
      // This is a bit complicated, as if all unique columns have an index, then
      // a compound index is wanted, but if only one of them then it is an ordinary index.
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

bool IDatabase::CreateRelationTables(const IModel &model) {
  bool create_ok = true;

  // Note that the error handling is divided into 2 type of errors.
  // The first type of errors are normal for incomplete
  // database models. The other type of error are type of errors that
  // is for complete models. The latter errors are treated as an error.

  const auto& relation_list = model.GetRelationList();
  for (const auto& [name, relation] : relation_list) {
    if (relation.Name().empty() || relation.DatabaseName().empty()) {
      LOG_INFO() << "Relation name is empty.";
      continue;
    }
    const auto* table1 = model.GetTable(relation.ApplicationId1());
    const auto* table2 = model.GetTable(relation.ApplicationId2());
    if (table1 == nullptr) {
      LOG_INFO() << "Relation table 1 doesn't exist. Relation: " << name
        << ", AID1: " << relation.ApplicationId1();
      continue;
    }
    if (table2 == nullptr) {
      LOG_INFO() << "Relation table 2 doesn't exist. Relation: " << name
           << ", AID2: " << relation.ApplicationId2();
      continue;
    }
    try {
      const auto* column1 = table1->GetColumnByBaseName("id");
      const auto* column2 = table2->GetColumnByBaseName("id");
      if (column1 == nullptr || column1->DatabaseName().empty()) {
        std::ostringstream err;
        err << "Relation table 1 doesn't have an index column. Table: " << table1->ApplicationName();
        throw std::runtime_error(err.str());
      }
      if (column2 == nullptr || column2->DatabaseName().empty()) {
        std::ostringstream err;
        err << "Relation table 2 doesn't have an index column. Table: " << table2->ApplicationName();
        throw std::runtime_error(err.str());
      }
      if ( IEquals(column1->DatabaseName(), column2->DatabaseName()) ) {
        std::ostringstream err;
        err << "The relation table indexes have the same column names. Relation: " << name
          << ", Column: " << column1->DatabaseName();
        throw std::runtime_error(err.str());
      }
      // Create the table
      std::ostringstream sql;
      sql << "CREATE TABLE IF NOT EXISTS " << relation.DatabaseName() << " ("
        << column1->DatabaseName() <<  " integer NOT NULL, "
        << column2->DatabaseName() <<  " integer NOT NULL, "
        << "REFNAME varchar DEFAULT '" << relation.Name() << "', "
        << "CONSTRAINT pk_" << relation.Name() << " PRIMARY KEY ("
        << column1->DatabaseName() << ","
        << column2->DatabaseName() << ",REFNAME) )";
      ExecuteSql(sql.str());
    } catch (const std::exception& err) {
      LOG_ERROR() << "Failed to create the relation tables. Error: " << err.what();
      create_ok = false;
    }
  }
  return create_ok;
}

std::string IDatabase::MakeCreateTableSql(const ods::IModel& model,
                                          const ods::ITable& table) {
  const auto& column_list = table.Columns();
  const auto unique_list = table.MakeUniqueList();
  bool use_serial = false; // Use serial for auto increment indexes
  switch (DatabaseType()) {
  case DbType::TypePostgres:
    use_serial = true;
    break;
  default:
    break;
  }
  std::ostringstream sql;
  sql << "CREATE TABLE IF NOT EXISTS " << table.DatabaseName() << " (";
  // Add primary key first (always the id column)
  bool first_column = true;
  for (const auto& iid : column_list) {
    if (iid.DatabaseName().empty() || !IEquals(iid.BaseName(), "id")) {
      continue;
    }

    if (use_serial) {
      sql << iid.DatabaseName() << " serial PRIMARY KEY";
    } else {
      sql << iid.DatabaseName() << " integer PRIMARY KEY AUTOINCREMENT";
    }
    first_column = false;
    break;
  }

  // Add the other columns
  for (const auto& column : column_list) {

    if (column.DatabaseName().empty() || IEquals(column.BaseName(), "id")) {
      continue;
    }
    if (first_column) {
      first_column = false;
    } else {
      sql << "," << std::endl;
    }

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

bool IDatabase::InsertModelEnvironment(const IModel &model) {
  const auto* env_table = model.GetTableByBaseId(BaseId::AoEnvironment);
  if (env_table == nullptr) {
    LOG_DEBUG() << "No environment table in DB. Assume no environment";
    return true;
  }

  const auto* name_column = env_table->GetColumnByBaseName("name");
  if (name_column == nullptr || env_table->DatabaseName().empty()) {
    LOG_DEBUG() << "No database name column in DB.";
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
    Insert(*env_table, env, SqlFilter());
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to insert model environment, Error: " << err.what();
    return false;
  }
  return true;
}

bool IDatabase::ReadModel(IModel &model) {
  DatabaseGuard guard(*this);
  if (!guard.IsOk()) {
    LOG_ERROR() << "Failed to open the database. Name: " << Name();
    return false;
  }

  const auto svc_enum = ReadSvcEnumTable(model);
  const auto svc_ent = ReadSvcEntTable(model);
  const auto svc_attr = ReadSvcAttrTable(model);
  const auto svc_ref = ReadSvcRefTable(model);

  const auto units = FixUnitStrings(model);
  const auto env = FetchModelEnvironment(model);

  // A common problem is that the unit and physical dimension tables,
  // not are case-sensitive.
  // Check that the name columns are case-sensitive.
  ITable* unit_table = model.GetTableByBaseId(BaseId::AoUnit);
  if (unit_table != nullptr) {
    IColumn* name_column = unit_table->GetColumnByBaseName("name");
    if (name_column != nullptr) {
      name_column->CaseSensitive(true);
    }
  }

  ITable* dim_table = model.GetTableByBaseId(BaseId::AoPhysicalDimension);
  if (dim_table != nullptr) {
    IColumn* name_column = dim_table->GetColumnByBaseName("name");
    if (name_column != nullptr) {
      name_column->CaseSensitive(true);
    }
  }

  return svc_enum && svc_ent && svc_attr && svc_ref && units && env;
}

size_t IDatabase::Count(const ITable &table, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open.");
  }
  if (table.DatabaseName().empty()) {
    return 0;
  }

  std::ostringstream sql;
  sql << "SELECT COUNT(*)  FROM " << table.DatabaseName() ;
  if (!filter.IsEmpty()) {
    sql << " " << filter.GetWhereStatement();
  }
  const auto ret_val = ExecuteSql(sql.str());
  return static_cast<size_t>(ret_val);
}

bool IDatabase::ExistDatabaseTable(const std::string &dbt_name) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open.");
  }
  if (dbt_name.empty()) {
    return false;
  }

  std::ostringstream sql;
  sql << "SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES "
    << "WHERE TABLE_NAME = '" << dbt_name << "'";
  const auto ret_val = ExecuteSql(sql.str());
  return ret_val > 0;
}

void IDatabase::AddComments(const ITable &table) {
  if (table.DatabaseName().empty()) {
    return;
  }
  try {
    // Add comments to the table
    const auto &table_desc = table.Description();

    auto* table_comment = sqlite3_mprintf("%Q",
                  table_desc.empty() ? nullptr : table_desc.c_str());
    std::ostringstream sql_table;
    sql_table << "COMMENT ON TABLE " << table.DatabaseName() << " IS "
              << table_comment;
    sqlite3_free(table_comment);
    ExecuteSql(sql_table.str());

    const auto& column_list = table.Columns();
    for (const auto& column : column_list) {
      if (column.DatabaseName().empty()) {
        continue;
      }
      const auto &column_desc = column.Description();

      auto* column_comment = sqlite3_mprintf("%Q",
                column_desc.empty() ? nullptr : column_desc.c_str());
      std::ostringstream sql_column;
      sql_column << "COMMENT ON COLUMN " << table.DatabaseName()
                 << "." << column.DatabaseName() << " IS "
                 << column_comment;
      sqlite3_free(column_comment);
      ExecuteSql(sql_column.str());
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to insert comments, Error: " << err.what();
  }
}

void IDatabase::ConnectionInfo(const std::string &info) {
  connection_info_ = info;
}

int64_t IDatabase::Exists(const ITable &table, const SqlFilter &filter) {
  if (filter.IsEmpty()) {
    return 0;
  }
  IdNameMap name_list;
  FetchNameMap(table, name_list, filter);
  if (name_list.empty()) {
    return 0;
  }
  const auto& [index, name] = *name_list.cbegin();
  return index;
}

void IDatabase::Insert(const ITable &table, IItem &row, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  const auto &column_list = table.Columns();
  const auto* column_id = table.GetColumnByBaseName("id");
  if (table.DatabaseName().empty() || column_list.empty() || column_id == nullptr) {
    return;
  }

  std::ostringstream insert;
  std::ostringstream values;
  insert << "INSERT INTO " << table.DatabaseName() << " (";
  bool first = true;
  for (const auto &col: column_list) {
    if (IEquals(col.BaseName(), "id") || col.DatabaseName().empty()) {
      continue;
    }
    if (!first) {
      insert << ",";
      values << ",";
    } else {
      first = false;
    }
    insert << col.DatabaseName();
    const auto *attr = row.GetAttribute(col.ApplicationName());
    if (attr != nullptr) {
      // The user has set the item
      switch (col.DataType()) {
      case DataType::DtDate:
        values << MakeDateValue(*attr);
        break;

      case DataType::DtString:
      case DataType::DtExternalRef: {
        const auto val = attr->Value<std::string>();
        if (val.empty() && !col.Obligatory() && col.DefaultValue().empty()) {
          values << "NULL";
        } else {
          auto *value = sqlite3_mprintf("%Q", val.c_str());
          values << value;
          sqlite3_free(value);
        }
        break;
      }

      default:
        if (col.ReferenceId() > 0 && attr->Value<int64_t>() <= 0) {
          values << "NULL";
        } else {
          const std::string value = attr->Value<std::string>();
          values << (!value.empty() ? value : "NULL");
        }
        break;
      }
    } else {
      // The user has not set item. First check the DtDate columns
      if (IEquals(col.BaseName(), "ao_created") ||
          IEquals(col.BaseName(), "version_date") ||
          IEquals(col.BaseName(), "ao_last_modified")) {
        // If these columns aren't set, then set them to 'now'.
        // Normal are they set to auto generate.
        // Note that 'ao_last_modified' is set to null at insert and set at update.
        const auto now = TimeStampToNs();
        const auto timestamp = NsToIsoTime(now, 0);
        values << "'" << timestamp << "'";
      } else if (!col.DefaultValue().empty()) {
        if (col.IsString()) {
          auto *value = sqlite3_mprintf("%Q", col.DefaultValue().c_str());
          values << value;
          sqlite3_free(value);
        } else {
          values << col.DefaultValue();
        }
      } else if (col.Obligatory()) {
        if (col.IsString()) {
          auto *value = sqlite3_mprintf("%Q", "");
          values << value;
          sqlite3_free(value);
        } else {
          values << "0";
        }
      } else {
        values << "NULL";
      }
    }
  }
  insert << ") VALUES (" << values.str() << ") RETURNING "
         << column_id->DatabaseName();

  const auto idx = ExecuteSql(insert.str());
  row.ItemId(idx);
}


void IDatabase::Update(const ITable &table, IItem &row, const SqlFilter& filter) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  const auto &column_list = table.Columns();
  if (table.DatabaseName().empty() || column_list.empty()) {
    return;
  }

  std::ostringstream update;
  update << "UPDATE " << table.DatabaseName() << " SET ";
  bool first = true;
  for (const auto &col: column_list) {
    if (col.DatabaseName().empty() || IEquals(col.BaseName(), "id")) {
      continue;
    }

    const auto *attr = row.GetAttribute(col.ApplicationName());
    if (attr != nullptr) {
      // Set the attribute value
      if (!first) {
        update << ",";
      } else {
        first = false;
      }
      update << col.DatabaseName() << "=";

      const std::string val = attr->Value<std::string>();
      if (col.DataType() == DataType::DtDate) {
        update << MakeDateValue(*attr);
      } else if (col.IsString()) {
        if (val.empty() && !col.Obligatory()) {
          update << "NULL";
        } else {
          auto *value = sqlite3_mprintf("%Q", val.c_str());
          update << value;
          sqlite3_free(value);
        }
      } else {
        update << val;
      }
    } else if (IEquals(col.BaseName(), "ao_last_modified") ||
               IEquals(col.BaseName(), "version_date")) {
      // Automatic fill with current date and time
      if (!first) {
        update << ",";
      } else {
        first = false;
      }
      update << col.DatabaseName() << "= datetime('now')";
    }
  }

  update << " " << filter.GetWhereStatement();
  ExecuteSql(update.str());
}

std::string IDatabase::DatabaseTypeAsString() const {
  switch (type_of_database_) {
  case DbType::TypeGeneric:
    return "Generic";

  case DbType::TypeSqlite:
    return "SQLite";

  case DbType::TypePostgres:
    return "Postgres";

  case DbType::TypeOracle:
    return "Oracle";

  case DbType::TypeSqlServer:
    return "SQLServer";

  default:
    break;
  }
  return "Unknown";
}

DbType IDatabase::StringAsDatabaseType(const std::string &type) {
  if (IEquals(type, "SQLite")) {
    return DbType::TypeSqlite;
  }
  if (IEquals(type, "Postgres")) {
    return DbType::TypePostgres;
  }
  if (IEquals(type, "Oracle")) {
    return DbType::TypeOracle;
  }
  if (IEquals(type, "SQLServer")) {
    return DbType::TypeSqlServer;
  }
  return DbType::TypeGeneric;
}

std::string IDatabase::DumpDatabase(const std::string &root_dir) {

  std::string dump_dir = CreateDumpDir(root_dir);
  if (dump_dir.empty()) {
    return dump_dir;
  }

  // Save the model file in the dump_dir
  IModel model;
  const bool read_model = ReadModel(model);
  if (!read_model) {
    dump_dir.clear();
    return dump_dir;
  }

  const bool save_model = SaveModelFile(dump_dir,model);
  if (!save_model) {
    dump_dir.clear();
    return dump_dir;
  }
  std::vector<std::string> fail_list;
  const auto table_list = model.AllTables();
  for (const ITable* table : table_list) {
    if (table == nullptr || table->DatabaseName().empty() ) {
      continue;
    }
    const bool dump = DumpTable(dump_dir, *table);
    if (!dump) {
      LOG_ERROR() << "Failed to dump a database table. Database: " << Name()
        << ". Table: " << table->DatabaseName();
      fail_list.emplace_back(table->DatabaseName());
    }
  }
  if (!fail_list.empty()) {
    dump_dir.clear();
  }

  return dump_dir;
}

std::string IDatabase::CreateDumpDir(const std::string &root_dir) const {
  std::string dump_dir;

  const std::string db_name = Name().empty() ? "default" : Name();
  // The database dump dir is <root_dir>/<DB name>_<YYYY-MM-DD_hh.mm.ss>
  std::ostringstream sub_dir;
  sub_dir << db_name << "_" << GetLocalTimeDirText(); // Include seconds

  try {
    const path root(root_dir);
    if (!exists(root)) {
      create_directories(root);
    }
    path dump(root);
    dump.append(sub_dir.str());
    // Check that this dir doesn't exist
    if (exists(dump)) {
      std::ostringstream error;
      error << "The new dump_dir already exist. Dir: " << dump;
      throw std::runtime_error(error.str());
    }
    dump_dir = dump.string();
    // Create the directory
    create_directory(dump);

  } catch (const std::exception& err) {
    LOG_ERROR() << "Cannot create dump directory. Dump Dir: " << dump_dir
    << ", Error: " << err.what();
    dump_dir.clear();
  }
  return dump_dir;
}

bool IDatabase::SaveModelFile(const std::string &dump_dir, const IModel &model) const {
  bool save = false;
  try {
    path model_file(dump_dir);
    std::string model_name = Name();
    if (model_name.empty()) {
      model_name = model.Name();
    }
    if (model_name.empty()) {
      model_name = "default";
    }
    model_name += ".xml";
    model_file.append(model_name);
    save = model.SaveModel(model_file.string());
  } catch( std::exception& error) {
    LOG_ERROR() << "Fail to save the model file. Error: " << error.what();
  }
  return save;
}

bool IDatabase::DumpTable(const std::string &dump_dir, const ITable &table) {
  if (table.DatabaseName().empty()) {
    return true;
  }

  DatabaseGuard db_open(*this);
  if (!db_open.IsOk()) {
    LOG_ERROR() << "Failed to open the database. Database: " << Name();
    return false;
  }

  SqlFilter fetch_all;
  const size_t nof_items = Count(table, fetch_all);
  if (nof_items == 0) {
    // no meaning to create a dump file if it's empty.
    return true;
  }

  // Create and open a dump file.
  std::string filename;
  try {
    path dump_file(dump_dir);
    std::string dump_name = table.DatabaseName() + ".dbt";
    std::transform(dump_name.cbegin(), dump_name.cend(), dump_name.begin(), ::tolower);
    dump_file.append(dump_name);
    filename = dump_file.string();
  } catch (const std::exception& err) {
    LOG_ERROR() << "Failed to create a dump file. File: " << filename << ", Error: " << err.what();
    return false;
  }
  std::ofstream out_file(filename,std::ios_base::out | std::ios_base::trunc );
  if (!out_file.is_open()) {
    LOG_ERROR() << "Failed to open the file. File: " << filename;
    return false;
  }
  size_t failed_rows = 0;
  const size_t nof_rows = FetchItems(table, fetch_all, [&] (IItem& row) -> void {
    const bool dump_row = DumpRow(table, row, out_file);
    if (!dump_row) {
      ++failed_rows;
    }
  });

  return failed_rows == 0 && nof_rows > 0;
}

bool IDatabase::DumpRow(const ITable &table, const IItem &row, std::ofstream &out_file) const {

  bool dump_row = true;
  for (const auto& attribute : row.AttributeList()) {
    const auto& name = attribute.Name();
    const auto* column = table.GetColumnByName(name);
    if (column == nullptr) {
      LOG_ERROR() << "Column not found in the database model. Dump mismatch. Table/Column: "
        << table.DatabaseName() << "/" << name;
      dump_row = false;
      continue;
    }

    if (attribute.IsValueEmpty() && !column->Obligatory() && !column->Unique()) {
      // This is a null value in a database.
      out_file << "~NULL~";
      out_file << "^";
      continue;
    }

    switch (column->DataType()) {
      case DataType::DtBoolean: {
        const bool value = attribute.Value<bool>();
        out_file << (value ? 1 : 0);
        break;
      }

      case DataType::DtBlob: {
        const auto blob_list = attribute.Value<std::vector<uint8_t>>();
        for (const uint8_t blob_value : blob_list) {
          char temp[10] = {'\0'};
          sprintf( temp, "%02X", static_cast<int>(blob_value) );
          out_file << temp;
        }
        break;
      }

      case DataType::DtByte: {
        out_file << attribute.Value<uint64_t>();
        break;
      }

      case DataType::DtEnum:
      case DataType::DtLongLong:
      case DataType::DtLong:
      case DataType::DtShort: {
        out_file << attribute.Value<int64_t>();
        break;
      }
      case DataType::DtDouble:
      case DataType::DtFloat: {
        const auto value = attribute.Value<double>();
        char temp[40] = {'\0'};
        std::to_chars(temp, temp + 38, value);
        temp[39] = '\0';
        out_file << temp;
        break;
      }

      case DataType::DtDate: {
        const uint64_t ns1970 = IsoTimeToNs(attribute.Value<std::string>(), false);
        int format = 0;
        if (ns1970 % 1'000 != 0) {
          format = 3;
        } else if (ns1970 % 1'000'000 != 0) {
          format = 2;
        } else if (ns1970 % 1'000'000'000 != 0) {
          format = 1;
        }
        out_file << NsToIsoTime(ns1970, format);
        break;
      }

      case DataType::DtExternalRef:
      case DataType::DtString:
      case DataType::DtId:
      case DataType::DtByteString:
      default: {
        out_file << OdsHelper::ConvertToDumpString(attribute.Value<std::string>());
        break;
      }
    }
    out_file << "^";
  }
  out_file << std::endl;
  return dump_row;
}

bool IDatabase::ReadInDump(const std::string &dump_dir) {
  // Verify that the dump directory exists and it has the required files.
  std::string model_file;
  std::map<std::string, std::string> dbt_list;
  const bool dump_files = ReadInDumpFiles(dump_dir, model_file, dbt_list);
  if (!dump_files) {
    LOG_ERROR() << "Failed to read in the database dump files. Directory: " << dump_dir;
    return false;
  }

  // Read in the dump model
  IModel dump_model;
  const bool read_dump_model = dump_model.ReadModel(model_file);
  if (!read_dump_model || dump_model.IsEmpty()) {
    LOG_ERROR() << "Invalid ODS dump model file. File: " << model_file;
    return false;
  }

  if (ConnectionInfo().empty()) {
    LOG_ERROR() << "The connection info cannot be empty. The database is not defined.";
    return false;
  }


  // Read in any existing model in the database.
  // Compare with the dump model and check if the database needs to be created.
  IModel db_model;
  const bool read_db_model = ReadModel(db_model);
  if (!read_db_model || db_model.IsEmpty()) {
    // The database needs to be created.
    const bool create_db = Create(dump_model);
    if (!create_db) {
      LOG_ERROR() << "Failed to create the database. Model: " << model_file;
      return false;
    }
  } else if (db_model != dump_model) {
    LOG_ERROR() << "The current database model differs from the dump model. Dump Model: "
      << model_file.empty();
    return false;
  }

  // Verify that the database is empty.
  // The read of dump is always done against an empty database.
  // Am import of a database, merges an old dump database to an existing database.
  const bool db_empty = IsEmpty(dump_model);
  if (!db_empty) {
    LOG_ERROR() << "The database is not empty. The dump cannot be read in from dump files.";
    return false;
  }

  // Read in all data from dump files into the database.
  const bool read_in_data = ReadInData(dump_model, dbt_list);
  return read_in_data;
}

bool IDatabase::ReadInDumpFiles(const std::string& dump_dir, std::string& model_file,
                     std::map<std::string, std::string>& dbt_list) {
  try {
    path dump(dump_dir);

    if (!exists(dump)) {
      throw std::runtime_error("Directory doesn't exists");
    }
    if (!is_directory(dump)) {
      throw std::runtime_error("This is not a directory");
    }
    for (const directory_entry& dir_entry : directory_iterator{dump_dir}) {
      if (!dir_entry.is_regular_file()) {
        continue;
      }
      const path& filename = dir_entry.path();
      const std::string extension = filename.extension().string();
      if (IEquals(extension, ".xml") && model_file.empty()) {
        model_file = filename.string();
        IModel test_model;
        const bool test = test_model.ReadModel(model_file);
        if (!test || test_model.IsEmpty()) {
          LOG_ERROR() << "Unwanted XML file found in dump directory. File: " << model_file;
          model_file.clear();
        }
      } else if (IEquals(extension, ".dbt")) {
        const std::string table_name = filename.stem().string();
        const std::string dbt_file = filename.string();
        dbt_list.emplace(table_name, dbt_file);
      }
    }
    if (model_file.empty()) {
      throw std::runtime_error("There is model file (*.xml) in the directory.");
    }
    if (dbt_list.empty()) {
      throw std::runtime_error("There is no DBT files in the directory.");
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "Invalid dump directory. Error: " << err.what() << ", Directory: " << dump_dir;
    return false;
  }
  return true;
}

bool IDatabase::IsEmpty(const IModel &model) {
  const auto table_list = model.AllTables();
  if (table_list.empty()) {
    LOG_ERROR() << "The database model has no tables. This is consider as an error. Model: " << model.Name();
    return false;
  }
  DatabaseGuard db_guard(*this);
  bool empty = true;
  for (const auto* table : table_list ) {
    if (table == nullptr || table->DatabaseName().empty()) {
      continue;
    }
    if (table->BaseId() == BaseId::AoEnvironment) {
      // Ignore environment as it might be filled by the create command
      continue;
    }
    SqlFilter empty_filter;
    const auto nof_rows = Count(*table, empty_filter);
    if (nof_rows > 0) {
      LOG_ERROR() << "A table have some rows. Table: " << table->DatabaseName() << ", Rows: " << nof_rows;
      empty = false;
    }
  }
  return empty;
}

bool IDatabase::ReadInData(const IModel &model, const std::map<std::string, std::string> &dbt_list) {

  bool read_in_data = true;
  for (const auto& [table_name, dbt_file] : dbt_list) {
    const auto* table = model.GetTableByDbName(table_name);
    if (table == nullptr) {
      table = model.GetTableByName(table_name);
    }
    if (table == nullptr) {
      read_in_data = false;
      LOG_INFO() << "Couldn't find the table in the model. Table: " << table_name;
      continue;
    }
    const bool read_table = ReadInTable(*table, dbt_file);
    if (!read_table) {
      read_in_data = false;
      LOG_ERROR() << "Failed to read in a dump file. File: " << dbt_file;
    }
  }

  return read_in_data;
}

void IDatabase::EnableIndexing(bool enable) {
  use_indexes_ = enable;
}

void IDatabase::EnableConstraints(bool enable) {
  use_constraints_ = enable;
}

bool IDatabase::ReadInTable(const ITable &table, const std::string &dbt_file) {
  bool read_in_table = true;
  EnableIndexing(false);
  EnableConstraints(false);

  DatabaseGuard db_lock(*this);
  if (!db_lock.IsOk()) {
    LOG_ERROR() << "Couldn't open the database. Database: " << Name();
    EnableIndexing(true);
    EnableConstraints(true);
    return false;
  }

  const IColumn *id_column = table.GetColumnByBaseName("id");

  size_t nof_rows = 0;
  size_t nof_fails = 0;
  try {

    std::ifstream file(dbt_file);
    if (!file.is_open()) {
      throw std::runtime_error("Couldn't open the DBT file.");
    }
    size_t unique_idx = 0; // Keeps track of suspicious indexes (<= 0)

    SqlFilter empty_filter;
    IItem row;
    for (bool more = OdsHelper::FetchDbtRow(table, row, file);
         more; more = OdsHelper::FetchDbtRow(table, row, file)) {
      // Need to validate the row.
      // Check id and name value
      if (row.AttributeList().empty()) {
        continue;
      }
      const int64_t idx = row.ItemId();
      const std::string name = row.Name();
      if (id_column != nullptr && id_column->Unique() && idx <= 0) {
        ++unique_idx;
        if (unique_idx >= 2) {
          continue;
        }
      }

       try {
        Insert(table, row, empty_filter);
      } catch (const std::exception &err) {
        LOG_ERROR() << "Failed read in a dump file. Error: " << err.what() << ", File: " << dbt_file;
        ++nof_fails;
      }
      ++nof_rows;
    } // end for loop

    if (unique_idx == 2) {
      LOG_INFO() << "Skipped " << unique_idx - 1 << " rows. Table/Idx: " << table.DatabaseName() << "/<0";
    }

  } catch (const std::exception &err) {
    LOG_ERROR() << "Failed read in a dump file. Error: " << err.what() << ", File: " << dbt_file;
    read_in_table = false;
  }

  EnableIndexing(true);
  EnableConstraints(true);
  if (nof_fails > 0 && nof_fails >= nof_rows) {
    read_in_table = false;
  }

  return read_in_table;
}

void IDatabase::InsertDumpRow(const ITable &table, IItem &row) {
  if (!IsOpen()) {
    throw std::runtime_error("The database is not open");
  }

  const std::vector<IColumn> &column_list = table.Columns();
  if (table.DatabaseName().empty() || column_list.empty() ) {
    return;
  }

  std::ostringstream insert;
  std::ostringstream values;
  insert << "INSERT INTO " << table.DatabaseName() << " (";
  bool first = true;
  for (const auto &col: column_list) {
    if (col.DatabaseName().empty()) {
      continue;
    }
    if (!first) {
      insert << ",";
      values << ",";
    } else {
      first = false;
    }
    insert << col.DatabaseName();
    const auto *attr = row.GetAttribute(col.ApplicationName());
    if (attr != nullptr) {
      // The user has set the item
      switch (col.DataType()) {
        case DataType::DtDate:
          values << MakeDateValue(*attr); // Note that this function normally is database-dependent.
          break;

        case DataType::DtString:
        case DataType::DtExternalRef: {
          const std::string val = attr->Value<std::string>();
          if (val.empty() && !col.Obligatory() && col.DefaultValue().empty()) {
            values << "NULL";
          } else {
            char* value = sqlite3_mprintf("%Q", val.c_str());
            values << (value != nullptr ? value : "NULL");
            sqlite3_free(value);
          }
          break;
        }

        case DataType::DtBlob: {// Convert the Base4 string to a hexadecimal string (X'hexbytes')
          const std::string base64 = attr->Value<std::string>();
          if (base64.empty()) {
            values << "NULL";
          } else {
            const std::vector<uint8_t> byte_array = OdsHelper::FromBase64(base64);
            const std::string hex_string = OdsHelper::ToHexString(byte_array);
            values << (!hex_string.empty() ? hex_string : "NULL");
          }
          break;
        }

        default:
          if (col.ReferenceId() > 0 && attr->Value<int64_t>() <= 0) {
            values << "NULL";
          } else {
            const std::string value = attr->Value<std::string>();
            values << (!value.empty() ? value : "NULL");
          }
          break;
      }
    } else {
      // The user has not set item. First check the DtDate columns
      if (IEquals(col.BaseName(), "ao_created") ||
          IEquals(col.BaseName(), "version_date") ||
          IEquals(col.BaseName(), "ao_last_modified")) {
        // If these columns aren't set, then set them to 'now'.
        // Normal are they set to auto generate.
        // Note that 'ao_last_modified' is set to null at insert and set at update.
        const uint64_t now = TimeStampToNs();
        const std::string timestamp = NsToIsoTime(now, 0);
        values << "'" << timestamp << "'";
      } else if (!col.DefaultValue().empty()) {
        if (col.IsString()) {
          auto *value = sqlite3_mprintf("%Q", col.DefaultValue().c_str());
          values << value;
          sqlite3_free(value);
        } else {
          values << col.DefaultValue();
        }
      } else if (col.Obligatory()) {
        if (col.IsString()) {
          auto *value = sqlite3_mprintf("%Q", "");
          values << value;
          sqlite3_free(value);
        } else {
          values << "0";
        }
      } else {
        values << "NULL";
      }
    }
  }
  insert << ") VALUES (" << values.str() << ")";

  ExecuteSql(insert.str());

}

} // end namespace ods

