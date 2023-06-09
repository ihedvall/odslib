/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include "ods/odsfactory.h"
#include "testdirectory.h"
#include "sqlitedatabase.h"
#include "postgresdb.h"
#include "sysloginserter.h"
#include "syslogrpcserver.h"
#include <util/stringutil.h>
#include <array>
#include "template_names.icc"

using namespace workflow;
using namespace util::string;

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

std::unique_ptr<workflow::IRunner>
OdsFactory::CreateTemplateTask(const workflow::IRunner &source) {
  std::unique_ptr<IRunner> runner;
  const auto& template_name = source.Template();
  if (IEquals(template_name, kSyslogInserter.data())) {
      auto temp = std::make_unique<SyslogInserter>(source);
      runner = std::move(temp);
  } else if (IEquals(template_name, kSyslogRpcServer.data())) {
      auto temp = std::make_unique<SyslogRpcServer>(source);
      runner = std::move(temp);
  }
  return runner;
}

void OdsFactory::CreateDefaultTemplateTask(workflow::WorkflowServer &server) {
  std::array<std::unique_ptr<IRunner>,2> temp_list = {
      std::make_unique<SyslogInserter>(),
      std::make_unique<SyslogRpcServer>(),
  };

  for (auto& temp : temp_list) {
      server.AddTemplate(*temp);
  }
}

} // end namespace