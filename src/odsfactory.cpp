/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include "ods/odsfactory.h"
#include "testdirectory.h"
#include "sqlitedatabase.h"
#include "postgresdb.h"

namespace ods {

std::unique_ptr<IEnvironment> OdsFactory::CreateEnvironment(
    EnvironmentType type) {
  std::unique_ptr<IEnvironment> env;
  switch (type) {
    case EnvironmentType::kTypeGeneric:
      break;

    case EnvironmentType::kTypeTestDirectory:
      env = std::move(std::make_unique<detail::TestDirectory>());
      break;

    default:
      break;
  }
  return env;
}

std::unique_ptr<IDatabase> OdsFactory::CreateDatabase(DbType type) {
  std::unique_ptr<IDatabase> database;
  switch (type) {
  case DbType::TypeSqlite: {
      auto temp = std::make_unique<detail::SqliteDatabase>();
      database = std::move(temp);
      break;
  }
  case DbType::TypePostgres: {
      auto temp = std::make_unique<detail::PostgresDb>();
      database = std::move(temp);
      break;
  }
  default:
      break;
  }
  return database;
}


} // end namespace