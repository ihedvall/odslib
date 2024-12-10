/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

/** \file imodel.h "ods/imodel.h"
 * \brief Defines an interface to a ODS database configuration.
 *
 * This file defines an interface against a generic ODS database.
 * The class is used to describe the application model of an ODS database.
 * The model and a database object, defines an ODS environment.
 * With these 2 objects, all types of operation can be done.
 * The purpose is mainly used when exporting, creating and importing of databases.
 * It can also be used to make application independent of the underlying database type.
 */
#pragma once
#include <map>
#include <memory>
#include <util/timestamp.h>
#include <util/stringutil.h>
#include <util/ixmlnode.h>

#include "ods/itable.h"
#include "ods/ienum.h"
#include "ods/irelation.h"

namespace ods {

class IModel {
 public:
  /** \brief The table list is just a list of tables sorted on table ID number. */
  using TableList = std::map<int64_t, ITable>;

  /** \brief List of enumerates. Sorted on enumerate names. */
  using EnumList = std::map<std::string, IEnum, util::string::IgnoreCase>;

  /** \brief List of cross-reference tables.
   *
   * The relation list defines the many-to-many relations.
   * These tables doesn't fit into the other table definitions.
   * The list is sorted by reference name.
   */
  using RelationList = std::map<std::string, IRelation, util::string::IgnoreCase>;

  //IModel() = default;
  //virtual ~IModel() = default;

  /** \brief The equality operator is mainly used for checking if the model have changed.
   *
   * The function is mainly used to check if the model changed.
   * This is typical used by a configuration GUI.
   * @param model Model to compare with
   * @return True if the models are the same.
   */
  [[nodiscard]] bool operator == (const IModel& model) const;

  /** \brief Model name is mainly for internal use.
   *
   * @return Returns a reference to the model name.
   */
  [[nodiscard]] const std::string& Name() const {
    return name_;
  }

  /** \brief Sets the model name.
   *
   * @param name New model name.
   */
  void Name(const std::string& name) {
    name_ = name;
  }

  /** \brief Returns the application version.
   *
   * @return Application version.
   */
  [[nodiscard]] const std::string& Version() const {
    return version_;
  }

  /** \brief Sets the application version.
   *
   * @param version Application version.
   */
  void Version(const std::string& version) {
    version_ = version;
  }

  /** Returns a description of the application model.
   *
   * @return Descriptive text.
   */
  [[nodiscard]] const std::string& Description() const {
    return description_;
  }

  /** Sets the description for the application model.
   *
   * @param desc Descriptive text.
   */
  void Description(const std::string& desc) {
    description_ = desc;
  }

  /** \brief Returns how created the model.
   *
   * @return Creator name.
   */
  [[nodiscard]] const std::string& CreatedBy() const {
    return created_by_;
  }

  /** \brief Sets the creator name.
   *
   * @param creator Creator name.
   */
  void CreatedBy(const std::string& creator) {
    created_by_ = creator;
  }

  /** \brief Returns the user name who did the last change.
   *
   * @return User name of the last change.
   */
  [[nodiscard]] const std::string& ModifiedBy() const {
    return modified_by_;
  }

  /** \brief Sets the user name of the last change.
   *
   * @param modifier User name (last change).
   */
  void ModifiedBy(const std::string& modifier) {
    modified_by_ = modifier;
  }

  /** \brief Returns the ASAM version in use.
   *
   * @return ASAM ODS version.
   */
  [[nodiscard]] const std::string& BaseVersion() const {
    return base_version_;
  }

  /** \brief Sets the ASAM base version.
   *
   * @param version ASAM ODS base version.
   */
  void BaseVersion(const std::string& version) {
    base_version_ = version;
  }

  /** \brief Creation time (ns since 1970 UTC)
   *
   * Creation time is defined as number of nano-seconds since 1970-01-01.
   * The time is always using UTC timezone.
   * @return Creation time (ns since 1970 UTC).
   */
  [[nodiscard]] uint64_t Created() const {
    return created_;
  }

  /** \brief Sets the creation time.
   *
   * @param ns1970 Creation time as ns since 1970 UTC.
   */
  void Created(uint64_t ns1970) {
    created_ = ns1970;
  }

  /** \brief Modification time as ns since 1970 UTC.
   *
   * Modification time is defined as number of nano-seconds since 1970-01-01.
   * The time is always using UTC timezone.
   * @return Modification time (ns since 1970 UTC).
   */
  [[nodiscard]] uint64_t Modified() const {
    return modified_;
  }

  /** \brief Sets the modification time.
   *
   * @param ns1970 Modification time as ns since 1970 UTC.
   */
  void Modified(uint64_t ns1970) {
    modified_ = ns1970;
  }

  /** \brief Name of the source where the model was fetched from.
   *
   * The model is either created by the ODS configurator but can
   * also be loaded from an existing database. This property holds
   * a reference that source.
   * @return Source name.
   */
  [[nodiscard]] const std::string& SourceName() const {
    return source_name_;
  }

  /** \brief Sets the source name.
   *
   * @param name Source name.
   */
  void SourceName(const std::string& name) {
    source_name_ = name;
  }

  /** \brief Returns the type of source.
   *
   * Returns the type of source.
   * @return Type of source.
   */
  [[nodiscard]] const std::string& SourceType() const {
    return source_type_;
  }

  /** \brief Sets the typeof source.
   *
   * @param type Type of source.
   */
  void SourceType(const std::string& type) {
    source_type_ = type;
  }

  /** \brief Returns source description.
   *
   * @return Source description.
   */
  [[nodiscard]] const std::string& SourceInfo() const {
    return source_info_;
  }

  /** \brief Sets the source description.
   *
   * @param info Source description.
   */
  void SourceInfo(const std::string& info) {
    source_info_ = info;
  }

  /** \brief Adds a top-most table to the model.
   *
   * This function can be used for new or modified tables.
   * The function uses the tables application ID as unique key.
   * Be careful to use the application ID 0 if the table should be added.
   * New tables are assigned new application ID.
   *
   * @param table Reference to the new or modified table.
   */
  void AddTable(const ITable& table);

  /** \brief Deletes a table using its application ID.
   *
   * @param application_id
   * @return Return true if the table was deleted.
   */
  bool DeleteTable(int64_t application_id);

  /** \brief Adds an enumerate to the model.
   *
   * Enumerate are integer to text maps.
   * The database stores an integer value while the end-user see a text.
   * The first value (please use 0) is traditionally the default value.
   *
   * If the enumerate exists, it will be replaced with the new enumerate.
   * The enumerate must have an unique name.
   *
   * @param obj New or modified enumerate.
   */
  void AddEnum(const IEnum& obj);

  /** \brief Removes an enumerate.
   *
   * @param name Name of the enumerate.
   */
  void DeleteEnum(const std::string& name);

  /** \brief Returns the a free enumerate ID.
   *
   * The ODS model doesn't use automatic index in the database.
   * Instead, an unique index is generated by this function.
   *
   * @return Next free enumerate ID number.
   */
  [[nodiscard]] int64_t FindNextEnumId() const;

  /** \brief Returns the enumerate list. */
  [[nodiscard]] const EnumList& Enums() const {
    return enum_list_;
  }

  /** \brief Returns the enumerate list. */
  [[nodiscard]] EnumList& Enums() {
    return enum_list_;
  }

  /** \brief Returns the next free table ID.
   *
   * Finding a unique application ID for a table can be somewhat
   * difficult. This function is mainly used internally to find
   * a free unique number.
   *
   * The parent ID is used as a hint to get a nice number.
   * @param parent_id Parent application ID.
   * @return
   */
  [[nodiscard ]] int64_t FindNextTableId(int64_t parent_id) const;

  /** \brief Returns a list of top-most tables.
   *
   * The function returns the top-most tables in the ODS model.
   * @return List of the top-most tables in the model.
   */
  [[nodiscard]] const TableList& Tables() const {
    return table_list_;
  }

  /** Clears all tables in the model. */
  void ClearTableList() {
    table_list_.clear();
  }

  /** \brief Returns a list of all tables in the model.
   *
   * This functions returns a list of table pointers for all tables in the model.
   * The list is sorted in dependence order. This is typical used when create the
   * database.
   *
   * @return A list of all tables in the model.
   */
  [[nodiscard]] std::vector<const ITable*> AllTables() const;

  /** \brief Returns an enumerate by its name. */
  [[nodiscard]] const IEnum* GetEnum(const std::string& name) const;

  /** \brief Returns an enumerate by its name. */
  [[nodiscard]] IEnum* GetEnum(const std::string& name);

  /** \brief Returns a table by its application ID. */
  [[nodiscard]] const ITable* GetTable(int64_t application_id) const;

  /** \brief Returns a table by its application name. */
  [[nodiscard]] const ITable* GetTableByName(const std::string& name) const;

  /** \brief Returns a table by its database name. */
  [[nodiscard]] const ITable* GetTableByDbName(const std::string& name) const;

  /** \brief Returns a table by its ODS base id. */
  [[nodiscard]] const ITable* GetTableByBaseId(BaseId base) const;

  /** \brief Returns a table by its ODS base id. */
  [[nodiscard]] ITable* GetTableByBaseId(BaseId base);

  /** \brief Returns all many-to-many relations. */
  [[nodiscard]] const RelationList& GetRelationList() const { return relation_list_; }

  /** \brief Returns all many-to-many relations. */
  [[nodiscard]] RelationList& GetRelationList() { return relation_list_; }

  /** \brief Adds or update an existing many-to-many relation. */
  void AddRelation(const IRelation& relation);

  /** \brief Deletes a many-to-many relation. */
  void DeleteRelation(const std::string& name);

  /** \brief Returns a many-to-many  by its application name. */
  [[nodiscard]] const IRelation* GetRelationByName(const std::string& name) const;

  /** \brief Returns true if its nothing in the model.
   *
   * @return True if the model is empty.
   */
  [[nodiscard]] bool IsEmpty() const;

  /** \brief Read in the model from a external XML configuration file.
   *
   * @param filename Full path to the configuration file.
   * @return True if the read was successful.
   */
  [[nodiscard]] bool ReadModel(const std::string& filename);

  /** \brief Stores the model into a XML file.
   *
   * The function stores the current configuration into an XML file.
   * If an earlier configuration file exist, it renames the previous
   * file and then stores the new configuration.
   * The rename algorithm stores the last 10 changes.
   * @param filename Full path to the configuration file.
   * @return
   */
  [[nodiscard]] bool SaveModel(const std::string& filename) const;
 private:
  std::string name_; ///< Application model name.
  std::string version_; ///< Application version.
  std::string description_; ///< Model description.

  std::string created_by_; ///< User who created the model.
  std::string modified_by_; ///< User of last change.
  std::string base_version_ = "asam35"; ///< ASAM ODS version.

  uint64_t created_ =  util::time::TimeStampToNs(); ///< Creation time.
  uint64_t modified_ = util::time::TimeStampToNs(); ///< Last change time.

  std::string source_name_; ///< Source name.
  std::string source_type_; ///< Source type.
  std::string source_info_; ///< Source information.

  TableList table_list_; ///< List of top-most tables.
  EnumList enum_list_; ///< List of all enumerates.
  RelationList  relation_list_; ///< List of cross-reference tables.

  /** \brief Internal function that reads in the enumerates. */
  void ReadEnum(const util::xml::IXmlNode& node);

  /** \brief Internal function that reads in the tables. */
  void ReadTable(const util::xml::IXmlNode& node);

  /** \brief Internal function that reads in the many to many tables. */
  void ReadRelation(const util::xml::IXmlNode& node);

  /** \brief Internal function that saves a many to many table configuration. */
  void SaveRelation(const IRelation& relation, util::xml::IXmlNode& root) const;
};

} // end namespace



