/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <ods/ienvironmentcreator.h>
#include <util/stringutil.h>
namespace ods::gui {

using EnvironmentList = std::map<std::string, std::unique_ptr<IEnvironment>, util::string::IgnoreCase>;

class EnvCreator : public ods::IEnvironmentCreator {
 public:
  [[nodiscard]] std::unique_ptr<IEnvironment> CreateEnvironment(EnvironmentType type) override;
  static std::unique_ptr<IEnvironment> CreateFromConfig(const std::string& name);
  static void SaveToConfig(const IEnvironment* env);

 private:

};

} // end namespace



