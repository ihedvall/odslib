/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <string>
#include "ods/imodel.h"
#include "ods/iitem.h"
namespace ods {

class AtfxFile {
 public:
  AtfxFile() = default;
  virtual ~AtfxFile() = default;
  [[nodiscard]] const std::string& FileName() const {
    return filename_;
  }

  void FileName(const std::string& filename);

  [[nodiscard]] const IModel& Model() const {
    return model_;
  }
  [[nodiscard]] bool Import();

 private:
  IModel model_;
  std::string filename_;
  std::vector<IItem> instance_data_;

  void ImportDocumentation(const util::xml::IXmlNode& node);
  void ImportAppModel(const util::xml::IXmlNode& node);
  void ImportInstanceData(const util::xml::IXmlNode& node);
  void ImportEnum(const util::xml::IXmlNode& node);
  void ImportElement(const util::xml::IXmlNode& node);
  void ImportRelation(const util::xml::IXmlNode& node, ITable& table);
  void ImportDataRow(const util::xml::IXmlNode& node);
  void FixTableRelations(const util::xml::IXmlNode& node);
  void FixRelation(const util::xml::IXmlNode& node, const ITable& table);
  void FixUnits();
};

} // end namespace




