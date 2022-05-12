/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include <filesystem>
#include <unordered_map>
#include <util/ixmlfile.h>
#include <util/logstream.h>
#include <util/stringutil.h>
#include "ods/atfxfile.h"
#include "ods/baseattribute.h"
#include "ods/idatabase.h"

using namespace util::xml;
using namespace util::log;
using namespace util::string;

namespace {

std::string MakeDbName(const std::string& hint) {
  std::ostringstream temp;
  for (const char cin : hint) {
    temp << static_cast<const char>(toupper(cin));
  }
  if (ods::IsSqlReservedWord(temp.str())) {
    temp << "X";
  }
  return temp.str();
}

std::string MakeDisplayName(const std::string& hint) {
  std::ostringstream temp;
  for (const char cin : hint) {
    if (std::isupper(cin) && !temp.str().empty()) {
      temp << " ";
    }
    temp << cin;
  }
  return temp.str();
}

void ImportAttribute(const util::xml::IXmlNode& node, ods::ITable& table) {
  const auto base_name = node.Property<std::string>("base_attribute");
  const auto app_name = node.Property<std::string>("name");
  auto column = ods::CreateDefaultColumn(table.BaseId(), base_name);
  column.ApplicationName(app_name);
  if (column.DatabaseName().empty()) {
    column.DatabaseName(MakeDbName(app_name));
  }
  if (column.DisplayName().empty()) {
    column.DisplayName(MakeDisplayName(app_name));
  }

  if (node.ExistProperty("datatype")) {
    column.DataType(ods::TextToDataType(node.Property<std::string>("datatype")));
  }
  if (node.ExistProperty("enumeration_type")) {
    column.EnumName(node.Property<std::string>("enumeration_type"));
  }
  if (node.ExistProperty("autogenerate")) {
    column.Auto(node.Property<bool>("autogenerate"));
  }
  if (node.ExistProperty("obligatory")) {
    column.Obligatory(node.Property<bool>("obligatory"));
  }
  if (node.ExistProperty("unique")) {
    column.Unique(node.Property<bool>("unique"));
  }
  column.DataLength(node.Property<size_t>("length"));
  column.UnitIndex(node.Property<int64_t>("unit"));
  table.AddColumn(column);
}

} // end namespace

namespace ods {

bool AtfxFile::Import() {
  auto file = CreateXmlFile();
  if (!file) {
    LOG_ERROR() << "Failed to create the XML parser.";
    return false;
  }
  file->FileName(filename_);
  const auto parse = file->ParseFile();
  if (!parse) {
    LOG_ERROR() << "Failed to parse the ATFX file. File: " << filename_;
    return false;
  }
  if (!IEquals(file->RootName(), "atfx_file")) {
    LOG_ERROR() << "Invalid root tag. Tag: " << file->RootName() << ", File: " << filename_;
    return false;
  }
  auto* doc = file->GetNode("documentation");
  if (doc != nullptr) {
    ImportDocumentation(*doc);
  }
  model_.BaseVersion(file->Property<std::string>("base_model_version"));

  auto* app = file->GetNode("application_model");
  if (app != nullptr) {
    ImportAppModel(*app);
  }
  auto* data = file->GetNode("instance_data");
  if (data != nullptr) {
    ImportInstanceData(*data);
  }
  FixUnits();
  return true;
}

void AtfxFile::FileName(const std::string &filename) {
  filename_ = filename;
  try {
    const std::filesystem::path full_path(filename_);
    const auto stem = full_path.stem();
    model_.SourceName(stem.string());
    model_.SourceType("ATFX");
    model_.SourceInfo(full_path.string());
  } catch (const std::exception& err) {
    LOG_ERROR() << "Path error. Error: " << err.what() << ", File: " << filename;
  }
}

void AtfxFile::ImportDocumentation(const IXmlNode &node) {
  IXmlNode::ChildList list;
  node.GetChildList(list);
  for (const auto* child : list) {
    if (child == nullptr) {
      continue;
    }
    if (child->IsTagName("exported_by")) {
      model_.CreatedBy(child->Value<std::string>());
    } else if (child->IsTagName("export_date_time")) {
      model_.Created(util::time::OdsDateToNs(child->Value<std::string>()));
    } else if (child->IsTagName("exporter_version")) {
      model_.Version(child->Value<std::string>());
    } else if (child->IsTagName("short_description") && model_.Description().empty()) {
      model_.Description(child->Value<std::string>());
    } else if (child->IsTagName("long_description") && model_.Description().empty()) {
      model_.Description(child->Value<std::string>());
    }
  }
  model_.Name(model_.SourceName());
  model_.ModifiedBy(model_.CreatedBy());
  model_.Modified(model_.Created());
}

void AtfxFile::ImportAppModel(const IXmlNode &node) {
  IXmlNode::ChildList list;
  node.GetChildList(list);
    // Import all enumerations first
  for (const auto* enumerate : list) {
    if (enumerate == nullptr) {
      continue;
    }
    if (enumerate->IsTagName("application_enumeration")) {
      ImportEnum(*enumerate);
    }
  }

  // We need to read in all table first without some relation columns.
  // This due to the referenced tables are referred before they are defined
  for (const auto* table : list) {
    if (table == nullptr) {
      continue;
    }
    if (table->IsTagName("application_element")) {
      ImportElement(*table);
    }
  }
  // This round we just fixes the references
  for (const auto* ref : list) {
    if (ref == nullptr) {
      continue;
    }
    if (ref->IsTagName("application_element")) {
      FixTableRelations(*ref);
    }
  }

}

void AtfxFile::ImportInstanceData(const IXmlNode &node) {
  IXmlNode::ChildList list;
  node.GetChildList(list);

  for (const auto* row : list) {
    if (row == nullptr) {
      continue;
    }
    ImportDataRow(*row);
  }
}

void AtfxFile::ImportEnum(const IXmlNode &node) {
  IEnum enumerate;
  enumerate.EnumName(node.Property<std::string>("name"));
  if (enumerate.EnumName().empty()) {
    return;
  }

  IXmlNode::ChildList list;
  node.GetChildList(list);
  for (const auto* child : list) {
    if (child == nullptr) {
      continue;
    }
    if (child->IsTagName("item")) {
      const auto name = child->Property<std::string>("name") ;
      const auto index = child->Property<int64_t>("value");
      enumerate.AddItem(index, name);
    }
  }
  model_.AddEnum(enumerate);
}

void AtfxFile::ImportElement(const IXmlNode &node) {
  ITable table;
  const auto app_name = node.Property<std::string>("name");
  table.ApplicationName(app_name);
  table.BaseId(TextToBaseId(node.Property<std::string>("basetype")));
  table.DatabaseName(MakeDbName(app_name));

  if (table.ApplicationName().empty()) {
    return;
  }

  IXmlNode::ChildList list;
  node.GetChildList(list);

  // Import columns first
  for (const auto* column : list) {
    if (column == nullptr) {
      continue;
    }
    if (column->IsTagName("application_attribute")) {
      ImportAttribute(*column, table);
    }
  }
  // Import relation
  for (const auto* rel : list) {
    if (rel == nullptr) {
      continue;
    }
    if (rel->IsTagName("relation_attribute")) {
      ImportRelation(*rel, table);
    }
  }
  model_.AddTable(table);
}

void AtfxFile::FixTableRelations(const util::xml::IXmlNode &node) {
  const auto table_name = node.Property<std::string>("name");
  const auto* table = model_.GetTableByName(table_name);
  if (table == nullptr) {
    return;
  }

  IXmlNode::ChildList list;
  node.GetChildList(list);

  // Fix relation
  for (const auto* rel : list) {
    if (rel == nullptr) {
      continue;
    }
    if (rel->IsTagName("relation_attribute")) {
      FixRelation(*rel, *table);
    }
  }
}

void AtfxFile::ImportRelation(const util::xml::IXmlNode& node, ITable& table) {
  const auto base_name = node.Property<std::string>("base_relation");
  const auto ref_to = node.Property<std::string>("ref_to");
  const auto* ref_table = ref_to.empty() ? nullptr : model_.GetTableByName(ref_to);
  const auto ref_id = ref_table == nullptr ? 0 : ref_table->ApplicationId();
  const auto min_occurs = node.Property<std::string>("min_occurs", "0");
  const auto max_occurs = node.Property<std::string>("max_occurs", "1");
  const auto app_name = node.Property<std::string>("name");

  const auto parent_list = GetParentBaseName(table.BaseId());
  const auto parent = std::ranges::any_of(parent_list, [&] (const auto& base) {
    return IEquals(base, base_name);
  });
  if (IEquals(max_occurs, "Many") || IEquals(min_occurs, "Many")) {
    // May be M:N relation
    return;
  }

  if (parent) {
    table.ParentId(ref_id);
  }

  auto column = ods::CreateDefaultColumn(table.BaseId(), base_name);
  column.ApplicationName(app_name);
  column.DataType(ods::DataType::DtLongLong);
  column.ReferenceId(ref_id);
  if (IEquals(min_occurs, "1") && IEquals(max_occurs, "1")) {
    column.Obligatory(true);
  } else if (IEquals(min_occurs, "0") && IEquals(max_occurs, "1")) {
    column.Obligatory(false);
  }
  if (parent) {
    column.Index(true);
  }
  if (column.DatabaseName().empty()) {
    column.DatabaseName(MakeDbName(app_name));
  }
  if (column.DisplayName().empty()) {
    column.DisplayName(MakeDisplayName(app_name));
  }
  table.AddColumn(column);
}

void AtfxFile::FixRelation(const util::xml::IXmlNode& node, const ITable& table) {
  const auto column_name = node.Property<std::string>("name");
  const auto* col = table.GetColumnByName(column_name);
  if (col == nullptr) {
    return;
  }
  const auto ref_to = node.Property<std::string>("ref_to");
  const auto* ref_table = ref_to.empty() ? nullptr : model_.GetTableByName(ref_to);
  if (ref_table == nullptr) {
    return;
  }
  const auto ref_id = ref_table == nullptr ? 0 : ref_table->ApplicationId();
  if (ref_id <= 0) {
    return;
  }
  auto* column = const_cast<IColumn*>(col);
  if (column == nullptr) {
    return;
  }
  column->ReferenceId(ref_id);
}

void AtfxFile::ImportDataRow(const util::xml::IXmlNode &node) {
  IItem row(node.TagName());
  IXmlNode::ChildList list;
  node.GetChildList(list);
  for (const auto* item : list) {
    if (!item) {
      continue;
    }

    const std::string& column = item->TagName();
    const std::string& value = item->Value<std::string>();
    IAttribute temp(column, value);
    row.AppendAttribute(temp);
  }
  instance_data_.emplace_back(row);
}

void AtfxFile::FixUnits() {
  std::unordered_map<int64_t, std::string> unit_list;
  const auto* unit_table = model_.GetBaseId(BaseId::AoUnit);
  if (unit_table == nullptr) {
    return;
  }
  auto* id_column = unit_table->GetColumnByBaseName("id");
  auto* name_column = unit_table->GetColumnByBaseName("name");
  if (id_column == nullptr || name_column == nullptr) {
    return;
  }

  for (const auto& data : instance_data_) {
    if (!IEquals(data.ApplicationName(), unit_table->ApplicationName())) {
      continue;
    }
    const auto* id_item = data.GetAttribute(id_column->ApplicationName());
    const auto* name_item = data.GetAttribute(name_column->ApplicationName());
    if (id_item == nullptr || name_item == nullptr) {
      continue;
    }
    unit_list.insert({id_item->Value<int64_t>(), name_item->Value<std::string>()});

  }
  auto table_list = model_.AllTables();
  for (const auto* tab : table_list) {
    if (tab == nullptr) {
      continue;
    }
    auto* table = const_cast<ITable*>(tab);
    if (table == nullptr) {
      continue;
    }
    auto& column_list = table->Columns();
    for (auto& column : column_list) {
      if (column.UnitIndex() <= 0) {
        continue;
      }
      const auto find = unit_list.find(column.UnitIndex());
      if (find != unit_list.cend()) {
        column.Unit(find->second);
      }
    }
  }
}

} // end namespace