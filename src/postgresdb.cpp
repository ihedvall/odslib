/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "postgresdb.h"
#include "postgresstatement.h"
#include <exception>
#include <util/logstream.h>
#include <util/utilfactory.h>
#include <util/stringutil.h>
#include <util/timestamp.h>
#include <sqlite3.h>
#include "ods/baseattribute.h"

using namespace util::log;
using namespace util::string;
using namespace util::time;

namespace ods::detail {

PostgresDb::PostgresDb()
 : listen_(util::UtilFactory::CreateListen("ListenProxy", "LISPOSTGRES")) {
  DatabaseType(DbType::TypePostgres);
}

PostgresDb::~PostgresDb() {
  PostgresDb::Close(false);
}

bool PostgresDb::HandleConnectionStringError() {
  // Check if connection string is OK.
  char* error_msg = nullptr;
  auto* resp = PQconninfoParse(ConnectionInfo().c_str(), &error_msg);
  if (resp != nullptr) {
    PQconninfoFree(resp);
  }
  const std::string error = error_msg != nullptr ? error_msg : "";
  if (error_msg != nullptr) {
      PQfreemem(error_msg);
  }
  if (!error.empty()) {
    LOG_ERROR() << "Connection string failure. Error: " << error;
    return false;
  }
  return true;
}

bool PostgresDb::HandleConnectionError() {
  // Check if connection string is OK.
  auto* error_msg = PQerrorMessage(connection_);
  const std::string error = error_msg != nullptr ? error_msg : "";
  // Note error_msg should not be freed
  if (!error.empty()) {
    LOG_ERROR() << "Connection failure. Error: " << error;
    return false;
  }
  return true;
}

bool PostgresDb::Open() {
  if (IsOpen()) {
    return true;
  }
  if (connection_ != nullptr) {
    PQfinish(connection_);
    connection_ = nullptr;
  }
  connection_ = PQconnectdb(ConnectionInfo().c_str());
  const auto status = PQstatus(connection_);
  if (status != CONNECTION_OK) {
    const auto conn_ok = HandleConnectionStringError();
    if (conn_ok) {
      HandleConnectionError();
    }
    PQfinish(connection_);
    connection_ = nullptr;
    return false;
  }
  if (listen_ && listen_->IsActive()) {
    // Todo Add listen connect info
  }
  try {
    ExecuteSql("BEGIN");
  } catch (const std::exception& err) {
    return false;
  }
  return true;
}

bool PostgresDb::IsOpen() const {
    return connection_ != nullptr;
}

bool PostgresDb::Close(bool commit) {
  if (!IsOpen()) {
    return true;
  }
  bool close = false;
  try {
    ExecuteSql(commit ? "COMMIT" : "ROLLBACK");
    close = true;
  } catch (const std::exception& error) {
    LOG_ERROR() << "Ending transaction failed. Error:" << error.what();
  }
  PQfinish(connection_);
  connection_ = nullptr;
  return close;
}

int64_t PostgresDb::ExecuteSql(const std::string &sql) {
  if (!IsOpen()) {
    throw std::runtime_error("Database not open");
  }
  int64_t ret_val = 0;
  auto *result = PQexec(connection_, sql.c_str());
  const auto status = PQresultStatus(result);
  switch (status) {
    case PGRES_EMPTY_QUERY:
    case PGRES_FATAL_ERROR:
    case PGRES_NONFATAL_ERROR:
    case PGRES_BAD_RESPONSE: {
      const auto *msg = PQresultErrorMessage(result);

      std::string error = msg != nullptr ? msg : "Bad response";
      std::ostringstream err;
      err << "Bad response on SQL. Error: " << error << ", SQL: " << sql;
      LOG_ERROR() << err.str();
      PQclear(result);
      throw std::runtime_error(err.str());
    }

    case PGRES_SINGLE_TUPLE:
    case PGRES_TUPLES_OK:
      if (result != nullptr) {
        const auto rows = PQntuples(result); // Should return 1
        const auto columns = PQnfields(result);
        if (columns > 0 && rows > 0) {
          const auto* value = PQgetvalue(result, 0,0);
          if (value != nullptr) {
            try {
              ret_val = std::stoll(value);
            } catch(const std::exception&) {}
          }
        }
      }
      break;

    default:
      break;

  }
  PQclear(result);
  return ret_val;
}

std::string PostgresDb::DataTypeToDbString(DataType type) {
  switch (type) {
  case DataType::DtShort:
  case DataType::DtByte:
      return "smallint";

  case DataType::DtBoolean:
      return "boolean";

  case DataType::DtEnum:
  case DataType::DtLong:
      return "integer";

  case DataType::DtId:
  case DataType::DtLongLong:
      return "bigint";

  case DataType::DtDouble:
      return "double precision";

  case DataType::DtFloat:
      return "real";

  case DataType::DtByteString:
  case DataType::DtBlob:
      return "bytea";

  case DataType::DtDate:
      return "timestamp(6) with time zone";

  case DataType::DtComplex:
      return "real[][2]";

  case DataType::DtDComplex:
      return "double precision[][2]";

  case DataType::DtExternalRef:
  case DataType::DtString:
      return "varchar";

  case DataType::DsString:
      return "varchar[]";

  case DataType::DsShort:
      return "smallint[]";
  default:
      break;
  }
  return "varchar";
}

bool PostgresDb::IsDataTypeString(DataType type) {
  return DataTypeToDbString(type) == "TEXT";
}


void PostgresDb::FetchNameMap(const ITable &table, IdNameMap& dest_list,
                              const SqlFilter& filter) {
  if (!IsOpen()) {
      throw std::runtime_error("The database is not open.");
  }

  const auto* column_id = table.GetColumnByBaseName("id");
  const auto* column_name = table.GetColumnByBaseName("name");
  if (table.DatabaseName().empty() || column_id == nullptr
      || column_name == nullptr ) {
      return;
  }

  std::ostringstream sql;
  sql << "SELECT " << column_id->DatabaseName() << "," << column_name->DatabaseName()
      << " FROM " << table.DatabaseName() ;
  if (!filter.IsEmpty()) {
      sql << " " << filter.GetWhereStatement();
  }

  PostgresStatement select(connection_, sql.str());
  for (bool more = select.Step(); more ; more = select.Step()) {
      const auto index = select.Value<int64_t>(0);
      const auto name = select.Value<std::string>(1);
      dest_list.emplace(index, name);
  }
}

void PostgresDb::FetchItemList(const ITable &table, ItemList& dest_list,
                               const SqlFilter& filter) {
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

  PostgresStatement select(connection_, sql.str());
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
size_t PostgresDb::FetchItems(const ITable &table, const SqlFilter &filter,
                              std::function<void(IItem &)> OnItem) {
  if (!IsOpen()) {
      throw std::runtime_error("The database is not open.");
  }
  if (table.DatabaseName().empty()) {
      return 0;
  }

  std::ostringstream sql;
  sql << "SELECT * FROM " << table.DatabaseName() ;
  if (!filter.IsEmpty()) {
      sql << " " << filter.GetWhereStatement();
  }
  size_t count = 0;
  PostgresStatement select(connection_, sql.str());
  for (bool more = select.Step(); more ; more = select.Step()) {
      const auto& column_list = table.Columns();
      IItem item;
      item.ApplicationId(table.ApplicationId());

      for (const auto& column : column_list) {
        const auto index = select.GetColumnIndex(column.DatabaseName());
        if (index < 0) {
          continue;
        }
        item.AppendAttribute({column.ApplicationName(), column.BaseName(), select.Value<std::string>(index)});
      }
      OnItem(item);
  }
  return count;
}

bool PostgresDb::ReadSvcEnumTable(IModel &model) {
  try {
      PostgresStatement select(connection_, "SELECT * FROM SVCENUM");
      for (bool more = select.Step(); more ; more = select.Step()) {
        const auto enum_id = select.GetColumnIndex("ENUMID");
        const auto enum_name = select.GetColumnIndex("ENUMNAME");
        const auto item = select.GetColumnIndex("ITEM");
        const auto item_name = select.GetColumnIndex("ITEMNAME");
        const auto locked = select.GetColumnIndex("LOCKED");

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

bool PostgresDb::ReadSvcEntTable(IModel &model) {
  try {
      PostgresStatement select(connection_, "SELECT * FROM SVCENT");

      for (bool more = select.Step(); more ; more = select.Step()) {
        ITable table;
        table.ApplicationId(select.Value<int64_t>("AID"));
        table.ApplicationName(select.Value<std::string>("ANAME"));
        table.BaseId(static_cast<BaseId>(select.Value<int>("BID")));
        table.DatabaseName(select.Value<std::string>("DBTNAME"));
        table.SecurityMode(select.Value<int64_t>("SECURITY"));
        table.Description(select.Value<std::string>("DESC"));
        table.ParentId(select.Value<int64_t>("FAID"));
        model.AddTable(table); // Note that the list is temporary,
                               // with no parent relations
      }
  } catch (std::exception& err) {
      LOG_ERROR() << "Failed to read the SVCENT table. Error: " << err.what();
      return false;
  }
  return true;
}

bool PostgresDb::ReadSvcAttrTable(IModel &model) {
  try {
      PostgresStatement select(connection_, "SELECT * FROM SVCATTR");
      for (bool more = select.Step(); more ; more = select.Step()) {
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

bool PostgresDb::FetchModelEnvironment(IModel &model) {

  try {
      /*
      // Pre-fill with file information in case no environment row. This is actually the normal case.
      const std::filesystem::path full_name(FileName());
      const auto last = std::filesystem::last_write_time(full_name);
      const auto ns1970 = util::time::FileTimeToNs(last);
      model.Modified(ns1970);
      model.Created(ns1970);
      model.Name(full_name.stem().string());
      model.SourceInfo(FileName());
*/
      const auto* env_table = model.GetBaseId(BaseId::AoEnvironment);
      if (env_table == nullptr || env_table->DatabaseName().empty()) {
        return true;
      }

      std::ostringstream sql;
      sql << "SELECT * FROM " << env_table->DatabaseName();
      PostgresStatement select(connection_, sql.str());
      for (bool more = select.Step(); more ; more = select.Step()) {
        model.Name(select.Value<std::string>(env_table->GetColumnByBaseName("name")));
        model.Version(select.Value<std::string>(env_table->GetColumnByBaseName("version")));
        model.Description(select.Value<std::string>(env_table->GetColumnByBaseName("description")));
        const auto *version_date = env_table->GetColumnByBaseName("version_date");
        if (version_date != nullptr) {
          model.Created(select.Value<uint64_t>(version_date));
          model.Modified(select.Value<uint64_t>(version_date));
        }

        model.CreatedBy(select.Value<std::string>(env_table->GetColumnByBaseName("ao_created_by")));
        const auto* created = env_table->GetColumnByBaseName("ao_created");
        if (created != nullptr) {
          model.Created(select.Value<uint64_t>(created));
        }

        model.ModifiedBy(select.Value<std::string>(env_table->GetColumnByBaseName("ao_modified_by")));
        const auto* modified = env_table->GetColumnByBaseName("ao_modified");
        if (modified != nullptr) {
          model.Modified(select.Value<uint64_t>(modified));
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



} // namespace ods::detail