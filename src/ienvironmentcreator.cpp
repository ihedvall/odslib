/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include "ods/ienvironmentcreator.h"
#include "testdirectory.h"

namespace ods {

std::unique_ptr<IEnvironment> IEnvironmentCreator::CreateEnvironment(ods::EnvironmentType type) {
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

} // end namespace