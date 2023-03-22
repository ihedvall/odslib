/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include "ods/ienvironment.h"
#include <util/stringutil.h>
namespace ods::gui {

using EnvironmentList = std::map<std::string, std::unique_ptr<IEnvironment>, util::string::IgnoreCase>;

class EnvCreator  {
 public:
  [[nodiscard]] static std::unique_ptr<IEnvironment> CreateEnvironment(EnvironmentType type);
  static std::unique_ptr<IEnvironment> CreateFromConfig(const std::string& name);
  static void SaveToConfig(const IEnvironment* env);

 private:

};

} // end namespace



