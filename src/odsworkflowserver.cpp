/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include "ods/odsworkflowserver.h"
#include <memory>
#include <array>
#include <util/stringutil.h>
#include "sysloginserter.h"

#include "template_names.icc"

using namespace workflow;
using namespace util::string;

namespace ods {

std::unique_ptr<IRunner>
OdsWorkflowServer::CreateRunner(const workflow::IRunner &source) {
  std::unique_ptr<IRunner> runner;
  const auto& template_name = source.Template();
  if (IEquals(template_name, kSyslogInserter.data())) {
    auto temp = std::make_unique<SyslogInserter>(source);
    runner = std::move(temp);
  }
  if (runner) {
    return runner;
  }
  return WorkflowServer::CreateRunner(source);
}

void OdsWorkflowServer::CreateDefaultTemplates() {
  std::array<std::unique_ptr<IRunner>,1> temp_list = {
      std::make_unique<SyslogInserter>(),
  };

  for (auto& temp : temp_list) {
    AddTemplate(*temp);
  }
  WorkflowServer::CreateDefaultTemplates();
}
} // namespace ods