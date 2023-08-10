/*
 * Copyright 2021 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "ods/idatabase.h"

#include <sqlite3.h>
#include <util/csvwriter.h>
#include <util/logstream.h>
#include <util/stringutil.h>
#include <util/timestamp.h>

#include "ods/databaseguard.h"

using namespace util::log;
using namespace util::string;
using namespace util::time;

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
    const auto* unit_table = model.GetBaseId(BaseId::AoUnit);
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
    if (ns1970 % 1'000 != 0) {
      format = 3;
    } else if (ns1970 % 1'000'000 != 0) {
      format = 2;
    } else if (ns1970 % 1'000'000'000 != 0) {
      format = 1;
    }
    temp = util::time::NsToIsoTime(ns1970,format);
  } else if (attr.IsValueEmpty()) {
    return "NULL";
  } else {
    temp = attr.Value<std::string>();
    if (IEquals(temp,"CURRENT_", 8)) {
      const auto now = TimeStampToNs();
      temp = NsToIsoTime(now, 0);
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
  // This method should only be used for tables with small amount of rows.
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
  const auto tables = CreateTables(model);
  const auto units = InsertModelUnits(model);
  const auto env = InsertModelEnvironment(model);
  const auto close = Close(true);
  return close && svc_enum && svc_ent && svc_attr && tables && units && env;
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
  IModel temp_model(model);
  try {
    ExecuteSql(kCreateSvcAttr.data());
    auto table_list = temp_model.AllTables();

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
  for (const auto& iid : column_list) {
    if (iid.DatabaseName().empty() || !IEquals(iid.BaseName(), "id")) {
      continue;
    }

    if (use_serial) {
      sql << iid.DatabaseName() << " serial PRIMARY KEY";
    } else {
      sql << iid.DatabaseName() << " integer PRIMARY KEY AUTOINCREMENT";
    }
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

bool IDatabase::InsertModelEnvironment(const IModel &model) {
  const auto* env_table = model.GetBaseId(BaseId::AoEnvironment);
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
  const auto units = FixUnitStrings(model);
  const auto env = FetchModelEnvironment(model);

  return svc_enum && svc_ent && svc_attr && units && env;
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
      // The item has been set by the user
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
          values << attr->Value<std::string>();
        }
        break;
      }
    } else {
      // Item has not been set by the user. First check the DtDate columns
      if (IEquals(col.BaseName(), "ao_created") ||
          IEquals(col.BaseName(), "version_date") ||
          IEquals(col.BaseName(), "ao_last_modified")) {
        // If these columns isn't set, then set them to 'now'.
        // Normally are these set to auto generated Note that
        // 'ao_last_modified' is set to null at insert and set at update.
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

} // end namespace ods

