/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include "ods/odsfactory.h"
#include "testdirectory.h"
#include "sqlitedatabase.h"
#include "postgresdb.h"
#include "sysloginserter.h"
#include <util/stringutil.h>
#include <array>
#include "template_names.icc"

using namespace workflow;
using namespace util::string;

namespace ods {
OdsFactory::OdsFactory() {
  name_ = "ODS Factory";
  description_ = "Tasks against an ODS database.";

  std::array<std::unique_ptr<IRunner>,1> template_list = {
      std::make_unique<SyslogInserter>(),
  };

  for ( auto& templ : template_list) {
    template_list_.emplace(templ->Name(), std::move(templ));
  }
}

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

std::unique_ptr<workflow::IRunner> OdsFactory::CreateRunner(const workflow::IRunner& source) const {
  std::unique_ptr<IRunner> runner;
  const auto& template_name = source.Template();
  if (IEquals(template_name, kSyslogInserter.data())) {
      auto temp = std::make_unique<SyslogInserter>(source);
      runner = std::move(temp);
  }
  return runner;
}

void OdsFactory::AddFactory(workflow::WorkflowServer &server) {
  server.AddRunnerFactory(*this);
}

OdsFactory &OdsFactory::Instance() {
  static OdsFactory instance;
  return instance;
}

} // end namespace