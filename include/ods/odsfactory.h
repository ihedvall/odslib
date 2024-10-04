/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include "ods/idatabase.h"
#include "ods/ienvironment.h"
#include <workflow/itaskfactory.h>
#include <workflow/workflowserver.h>
namespace ods {

class OdsFactory : public workflow::ITaskFactory {
 public:
  [[nodiscard]] static std::unique_ptr<IEnvironment>
                CreateEnvironment(EnvironmentType type);

  [[nodiscard]] static std::unique_ptr<IDatabase>
                CreateDatabase(DbType type);

  [[nodiscard]] std::unique_ptr<workflow::ITask> CreateTask(
      const workflow::ITask& source) const override;

   void AddFactory(workflow::WorkflowServer& server);

   static OdsFactory& Instance();
 protected:
   OdsFactory();
};

} // end namespace



