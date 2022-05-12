/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>
#include "ods/odsdef.h"

using namespace ods;

namespace ods::test {

TEST(OdsDef, TestBaseIdConversion) { // NOLINT
  EXPECT_EQ(TextToBaseId("45"), BaseId::AoParameterSet);
  EXPECT_EQ(TextToBaseId("ParameterSet"), BaseId::AoParameterSet);
  EXPECT_EQ(TextToBaseId("aoparameterset"), BaseId::AoParameterSet);

  EXPECT_EQ(TextToBaseId("38"), BaseId::AoSubMatrix);
  EXPECT_EQ(TextToBaseId("Submatrix"), BaseId::AoSubMatrix);
  EXPECT_EQ(TextToBaseId("SubMatrix"), BaseId::AoSubMatrix);

  const auto temp = BaseIdToText(BaseId::AoExternalComponent);
  EXPECT_EQ(TextToBaseId(temp), BaseId::AoExternalComponent);
}

TEST(OdsDef, TestDataTypeConversion) { // NOLINT
  EXPECT_EQ(TextToDataType("7"), DataType::DtDouble);
  EXPECT_EQ(TextToDataType("DtDouble"), DataType::DtDouble);
  EXPECT_EQ(TextToDataType("DT_DOUBLE"), DataType::DtDouble);
  EXPECT_EQ(TextToDataType("Double"), DataType::DtDouble);

  const auto temp = DataTypeToText(DataType::DtString);
  EXPECT_EQ(TextToDataType(temp), DataType::DtString);
}

}