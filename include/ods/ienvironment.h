/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <string>
#include "ods/odsdef.h"
#include "ods/imodel.h"

namespace ods {

class IEnvironment {
 public:
  virtual ~IEnvironment() = default;

  [[nodiscard]] const std::string& Name() const {
    return name_;
  }
  void Name(const std::string& name) {
    name_ = name;
  }

  [[nodiscard]] const std::string& Description() const {
    return description_;
  }
  void Description(const std::string& desc) {
    description_ = desc;
  }

  [[nodiscard]] EnvironmentType Type() const {
    return env_type_;
  }

  [[nodiscard]] const std::string& ModelFileName() const {
    return model_file_;
  }
  void ModelFileName(const std::string& model_file) {
    model_file_ = model_file;
  }

  [[nodiscard]] const IModel& Model() const {
    return model_;
  }
  [[nodiscard]] virtual bool IsOk() const = 0;
  [[nodiscard]] virtual bool Init() = 0;  ///< Initialize the environment

  [[nodiscard]] virtual bool IsStarted() const = 0;
  virtual void Start() = 0; ///< Start worker thread
  virtual void Stop() = 0;  ///< Stop the worker thread

  virtual bool FetchNameIdMap(const ITable& table, NameIdMap& dest_list) = 0;

 protected:
  IModel model_; ///< Most environment types are based upon an ODS model.
  explicit IEnvironment(EnvironmentType type);

 private:
  EnvironmentType env_type_ = EnvironmentType::kTypeGeneric;
  std::string name_;        ///< Name of the environment
  std::string description_; ///< Description of the environment
  std::string model_file_; ///< Full path to the model XML file.


  IEnvironment() = default;
};

} // end namespace




