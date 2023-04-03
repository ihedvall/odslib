/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include "ods/idatabase.h"
#include "ods/ienvironment.h"
#include <workflow/irunner.h>
#include <workflow/workflowserver.h>
namespace ods {

class OdsFactory {
 public:
  OdsFactory() = default;

  [[nodiscard]] static std::unique_ptr<IEnvironment>
                CreateEnvironment(EnvironmentType type);

  [[nodiscard]] static std::unique_ptr<IDatabase>
                CreateDatabase(DbType type);

  [[nodiscard]] static std::unique_ptr<workflow::IRunner>
                CreateTemplateTask(const workflow::IRunner& source);

   static void CreateDefaultTemplateTask(workflow::WorkflowServer& server);
};

} // end namespace



