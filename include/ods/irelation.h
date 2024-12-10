/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

/** \file irelation.h
 * \brief Interface against many to many relations tables.
 *
 * The SVCREF table holds the many to many relation table
 * description. These tables cannot be described in the
 * other SVC tables.
 *
 * A many to many relation table consist of at least 3 columns.
 * The first column is the index to the first relation while the second
 * column is the index to the second relation. The third column is the reference
 * name table name which is a redundant column. It is always default to reference
 * table name.
 *
 */
#pragma once
#include <cstdint>
#include <string>

namespace ods {

/** \class IRelation irelation.h "ods/irelation.h"
 * \brief Interface against a many to many relation table.
 *
 * The many to many relation tables cannot be defines in the other
 * SVC tables. Instead is a special SVCREF table used to define
 * these tables.
 *
 * The Many to many cross-reference table consist of 3 columns.
 * The first column is the relation 1 index.
 * The second column is the relation 2 index.
 * The third is fixed to the reference name. This column is unused.
 * I think you may have many many-to-many types of relation in the same
 * table but this is just bad design.
 *
 * The two first column names should be identical to the referenced
 * application index column names. This means that the default IDX name
 * needs to be changed on at least one of the columns.
 * Maybe 'table_name'_idx is
 * a recommended name on the column.
 */
class IRelation {
 public:

  [[nodiscard]] bool operator == (const IRelation& relation) const;
  /** \brief Sets the application name of the many to many relation table.
   *
   * @param name Application name of the table
   */
  void Name(const std::string& name){ name_ = name; }

  /** \brief Returns the application table name.
   *
   * @return Reference to application table name.
   */
  [[nodiscard]] const std::string& Name() const { return name_; }

  /** \brief Sets the application ID of referenced table 1.
   *
   * @param app_id Unique application ID for the referenced table.
   */
  void ApplicationId1(int64_t app_id ) {application_id1_ = app_id; }

  /** \brief Returns the referenced table application ID.
   *
   * @return Application ID of referenced table 1
   */
  [[nodiscard]] int64_t ApplicationId1() const { return application_id1_; }

  /** \brief Sets the application ID of referenced table 2.
  *
  * @param app_id Unique application ID for the referenced table.
  */
  void ApplicationId2(int64_t app_id ) {application_id2_ = app_id; }

  /** \brief Returns the referenced table application ID.
   *
   * @return Application ID of referenced table 1
   */
  [[nodiscard]] int64_t ApplicationId2() const { return application_id2_; }

  /** \brief Sets the database name of the many to many relation table.
   *
   * @param name Database name of the table
   */
  void DatabaseName(const std::string& name){ database_name_ = name; }

  /** \brief Returns the database table name.
   *
   * @return Reference to the database table name.
   */
  [[nodiscard]] const std::string& DatabaseName() const { return database_name_; }

/** \brief Sets the inverse name of the many to many relation table.
   *
   * Inverse or circular dependencies are not used nor recommended. This
   * property is not used by the library but included in the ODS standard.
   * @param name Inverse name of the table
   */
  void InverseName(const std::string& name){ inverse_name_ = name; }

  /** \brief Returns the inverse name.
   *
   * @return Reference to the inverse name.
   */
  [[nodiscard]] const std::string& InverseName() const { return inverse_name_; }

  /** \brief Sets the base name of the many to many relation table.
   *
   * @param name Base name of the table
   */
  void BaseName(const std::string& name){ base_name_ = name; }

  /** \brief Returns the base name.
   *
   * @return Reference to the base name.
   */
  [[nodiscard]] const std::string& BaseName() const { return base_name_; }

  /** \brief Sets the inverse base name of the many to many relation table.
 *
 * @param name Inverse base ase name of the table
 */
  void InverseBaseName(const std::string& name){ inverse_base_name_ = name; }

  /** \brief Returns the inverse base name.
   *
   * @return Reference to the inverse base name.
   */
  [[nodiscard]] const std::string& InverseBaseName() const { return inverse_base_name_; }

 private:
  std::string name_; ///< Application name of the relation.
  int64_t application_id1_ = 0; ///< Application ID for relation 1.
  int64_t application_id2_ = 0; ///< Application ID for relation 2.
  std::string database_name_; ///< Database table name.
  std::string inverse_name_;  ///< Inverse name.
  std::string base_name_; ///< Base name of the relation.
  std::string inverse_base_name_; ///< Inverse base name

};

} // end namespace




