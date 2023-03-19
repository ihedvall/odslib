/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <filesystem>
#include <util/logstream.h>
#include <util/stringutil.h>
#include "ods/ienvironment.h"

using namespace util::log;
using namespace std::filesystem;
using namespace util::string;

namespace ods {

IEnvironment::IEnvironment(EnvironmentType type)
: env_type_(type) {

}

bool IEnvironment::CreateDb() {
  if (ModelFileName().empty()) {
    LOG_ERROR() << "No model file defined. Cannot create a database.";
    return false;
  }
  const auto read = model_.ReadModel(ModelFileName());
  if (!read) {
    LOG_ERROR() << "Failed to read in the model. File: " << ModelFileName();
    return false;
  }
  const auto create = Database().Create(model_);
  if (!create) {
    LOG_ERROR() << "Failed to create the cache database.";
    return false;
  }
  return true;
}

bool IEnvironment::InitDb() {
  if (model_.IsEmpty()) {
    const auto read = Database().ReadModel(model_);
    if (!read) {
      LOG_ERROR() << "Failed to read in the ODS model from the database";
      return false;
    }
  }
  return true;
}

int64_t IEnvironment::AddUniqueName(const IItem &item) {
  auto row = item;
  const auto item_name = row.Name();
  const auto* table = row.ApplicationId() > 0 ?
      model_.GetTable(row.ApplicationId()) : model_.GetTableByName(row.ApplicationName());
  const auto* column_id = table != nullptr ? table->GetColumnByBaseName("id") : nullptr;
  const auto* column_name = table != nullptr ? table->GetColumnByBaseName("name") : nullptr;
  if (item_name.empty() || table == nullptr || column_name == nullptr || column_id == nullptr)  {
    return 0;
  }

  if (!row.ExistBaseAttribute("id")) {
    row.AppendAttribute({column_id->ApplicationName(), column_id->BaseName(), row.ItemId()});
  }
  if (!row.ExistBaseAttribute("name")) {
    row.AppendAttribute({column_name->ApplicationName(), column_name->BaseName(), item_name});
  }

  try {
    SqlFilter name_filter;
    name_filter.AddWhere(*column_name, SqlCondition::EqualIgnoreCase,item_name);

    Database().Insert(*table, row, name_filter);

    return row.ItemId();
  } catch (const std::exception& err) {
    LOG_ERROR() << "Insert failure. Error: " << err.what();
  }
  return 0;
}

bool IEnvironment::DumpDb(const std::string &dump_path) {
  // 1. Create directory.
  // 2. Create model xml file.
  // 3. Create a dbt file for each ODS table (not svc).

  if (Name().empty()) {
    LOG_ERROR() << "The environment (model) doesn't have a name. Cannot dump the model";
    return false;
  }

  try {
     path dump_dir(dump_path);
     // If the directory exist and not is empty, rename the directory and add a date/time onto its name
     if (exists(dump_path)) {

       bool need_renaming = false;
       for (const auto& entry : directory_iterator(dump_dir)) {
         const auto& file_path = entry.path();
         if (entry.is_directory()) {
           LOG_ERROR() << "The destination directory exist with a sub-directory. This not allowed. Path: "
            << file_path;
           return false;
         }
         if (IEquals(file_path.extension().string(), ".xml") &&
             IEquals(file_path.stem().string(), Name())) {
           LOG_DEBUG() << "Model file already exist. Backing up the directory";
           need_renaming = true;
         }
         if (IEquals(file_path.extension().string(), ".csv")) {
           if (!need_renaming) {
             LOG_DEBUG() << "Directory have some CSV file. Most likely a dump directory. CSV: "
              << file_path;
           }
           need_renaming = true;
         }


       }
       if (need_renaming) {
         path new_dir(dump_path);
         const auto now = util::time::NsToIsoTime(util::time::TimeStampToNs(), 0);
         new_dir += "_";
         new_dir += now;
         rename(dump_dir, new_dir);
         create_directories(dump_path);
       }
     } else {
       // Doesn't exist. Well create it.
       create_directories(dump_path);
     }
   } catch (const std::exception& err) {
    LOG_ERROR() << "File access error. Error: " << err.what() << ", Path: " << dump_path;
    return false;
  }

  // 2. Create the model XML file
  try {
    path file_model(dump_path);
    file_model /= Name();
    file_model += ".xml";
    const auto save = model_.SaveModel(file_model.string());
    if (!save) {
      LOG_ERROR() << "Failed to save the model file. Model: " << file_model;
      return false;
    }
  } catch (const std::exception& err) {
    LOG_ERROR() << "File access error. Error: " << err.what() << ", Path: " << dump_path;
    return false;
  }

  // 3. Dump all the ODS tables
  const auto& table_list = model_.AllTables();
  for (const auto* table : table_list) {
    if (table == nullptr || table->DatabaseName().empty() || table->ApplicationName().empty()) {
      continue;
    }
    try {
      path file_table(dump_path);
      file_table /= table->ApplicationName();
      file_table += ".csv";

      Database().ExportCsv(file_table.string(), *table, SqlFilter());
    } catch (const std::exception& err) {
      LOG_ERROR() << "Table CSV export failure. Error: " << err.what() << ", Path: " << dump_path;
      return false;
    }

  }
  return true;
}

} // end namespace