/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "ods/irelation.h"

namespace ods {

bool IRelation::operator==(const IRelation &relation) const {
  if (name_ != relation.name_) {
    return false;
  }
  if (application_id1_ != relation.application_id1_) {
    return false;
  }
  if (application_id2_ != relation.application_id2_) {
    return false;
  }
  if (database_name_ != relation.database_name_) {
    return false;
  }
  if (inverse_name_ != relation.inverse_name_) {
    return false;
  }
  if (base_name_ != relation.base_name_) {
    return false;
  }
  if (inverse_base_name_ != relation.inverse_base_name_) {
    return false;
  }
  return true;
}

} // end namespace
