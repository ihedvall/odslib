/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <workflow/workflowserver.h>
#include <workflow/irunner.h>
namespace ods {

class OdsWorkflowServer : public workflow::WorkflowServer {
public:
  std::unique_ptr<workflow::IRunner>
  CreateRunner(const workflow::IRunner &source) override;
  void CreateDefaultTemplates() override;

};

} // namespace ods
