/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include "ods/ienvironment.h"
namespace ods {

class IEnvironmentCreator {
 public:
  IEnvironmentCreator() = default;
  [[nodiscard]] virtual std::unique_ptr<IEnvironment> CreateEnvironment(EnvironmentType type);


};

} // end namespace



