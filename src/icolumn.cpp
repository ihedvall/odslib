/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

#include "ods/icolumn.h"

namespace ods {

bool IColumn::IsString() const {
  switch (data_type_) {
    case DataType::DtDate:
    case DataType::DtString:
    case DataType::DtExternalRef:
      return true;
    default:
      break;
  }
  return false;
}
}