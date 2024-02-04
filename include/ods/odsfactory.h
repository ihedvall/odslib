/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include "ods/idatabase.h"
#include "ods/ienvironment.h"
#include <workflow/irunnerfactory.h>
#include <workflow/workflowserver.h>
namespace ods {

class OdsFactory : public workflow::IRunnerFactory {
 public:
  [[nodiscard]] static std::unique_ptr<IEnvironment>
                CreateEnvironment(EnvironmentType type);

  [[nodiscard]] static std::unique_ptr<IDatabase>
                CreateDatabase(DbType type);

  [[nodiscard]] std::unique_ptr<workflow::IRunner> CreateRunner(
      const workflow::IRunner& source) const override;

   void AddFactory(workflow::WorkflowServer& server);

   static OdsFactory& Instance();
 protected:
   OdsFactory();
};

} // end namespace



