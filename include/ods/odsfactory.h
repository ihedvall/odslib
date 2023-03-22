/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include "ods/idatabase.h"
#include "ods/ienvironment.h"
namespace ods {

class OdsFactory {
 public:
  OdsFactory() = default;

  [[nodiscard]] static std::unique_ptr<IEnvironment>
                CreateEnvironment(EnvironmentType type);

  [[nodiscard]] static std::unique_ptr<IDatabase>
                CreateDatabase(DbType type);

};

} // end namespace



