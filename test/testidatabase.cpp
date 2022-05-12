/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>
#include "ods/idatabase.h"

namespace ods::test {

TEST(IDatabase, TestReservedWord) {
  EXPECT_TRUE(IsSqlReservedWord("abort"));
  EXPECT_TRUE(IsSqlReservedWord("ABORT"));
  EXPECT_FALSE(IsSqlReservedWord("aborting"));
  EXPECT_FALSE(IsSqlReservedWord("ABORTING"));
}

TEST(IDatabase, TestBaseNameColumn) {
  EXPECT_FALSE(IsSqlReservedWord("IID"));
  EXPECT_FALSE(IsSqlReservedWord("NAME"));
  EXPECT_FALSE(IsSqlReservedWord("DESCRIPTION"));
  EXPECT_FALSE(IsSqlReservedWord("VERSION"));
  EXPECT_FALSE(IsSqlReservedWord("STORED"));
  EXPECT_FALSE(IsSqlReservedWord("MIME_TYPE"));
  EXPECT_FALSE(IsSqlReservedWord("OBJECT_ID"));
  EXPECT_FALSE(IsSqlReservedWord("CREATED"));
  EXPECT_FALSE(IsSqlReservedWord("CREATED_BY"));
  EXPECT_FALSE(IsSqlReservedWord("MODIFIED"));
  EXPECT_FALSE(IsSqlReservedWord("MODIFIED_BY"));

  EXPECT_FALSE(IsSqlReservedWord("TEST_LEVEL"));
  EXPECT_FALSE(IsSqlReservedWord("BASE_MODEL"));
  EXPECT_FALSE(IsSqlReservedWord("APP_TYPE"));
  EXPECT_FALSE(IsSqlReservedWord("APP_VERSION"));
  EXPECT_FALSE(IsSqlReservedWord("TIMEZONE"));

  EXPECT_FALSE(IsSqlReservedWord("ENTITY_NAME"));
  EXPECT_FALSE(IsSqlReservedWord("ATTR_NAME"));
  EXPECT_FALSE(IsSqlReservedWord("PARENT"));

  EXPECT_FALSE(IsSqlReservedWord("FILE_TYPE"));
  EXPECT_FALSE(IsSqlReservedWord("FILE_LOCATION"));
  EXPECT_FALSE(IsSqlReservedWord("FILE_SIZE"));
  EXPECT_FALSE(IsSqlReservedWord("FILE_ORIG"));
  EXPECT_FALSE(IsSqlReservedWord("DATE_ORIG"));

  EXPECT_FALSE(IsSqlReservedWord("MQ_NAME"));
  EXPECT_FALSE(IsSqlReservedWord("DATA_TYPE"));
  EXPECT_FALSE(IsSqlReservedWord("RANK"));
  EXPECT_FALSE(IsSqlReservedWord("TYPE_SIZE"));
  EXPECT_FALSE(IsSqlReservedWord("UNIT"));
  EXPECT_FALSE(IsSqlReservedWord("FATHER"));

  EXPECT_FALSE(IsSqlReservedWord("FACTOR"));
  EXPECT_FALSE(IsSqlReservedWord("OFFSET_X"));
  EXPECT_FALSE(IsSqlReservedWord("PHYS_DIM"));

  EXPECT_FALSE(IsSqlReservedWord("LENGTH_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("MASS_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("TIME_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("CURR_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("TEMP_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("MOLAR_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("LUM_EXP"));
  EXPECT_FALSE(IsSqlReservedWord("LENGTH_EXP_DEN"));
  EXPECT_FALSE(IsSqlReservedWord("MASS_EXP_DEN"));
  EXPECT_FALSE(IsSqlReservedWord("TIME_EXP_DEN"));
  EXPECT_FALSE(IsSqlReservedWord("CURR_EXP_DEN"));
  EXPECT_FALSE(IsSqlReservedWord("TEMP_EXP_DEN"));
  EXPECT_FALSE(IsSqlReservedWord("MOLAR_EXP_DEN"));
  EXPECT_FALSE(IsSqlReservedWord("LUM_EXP_DEN"));

  EXPECT_FALSE(IsSqlReservedWord("M_START"));
  EXPECT_FALSE(IsSqlReservedWord("M_STOP"));
  EXPECT_FALSE(IsSqlReservedWord("ACC"));
  EXPECT_FALSE(IsSqlReservedWord("ACC_BY"));
  EXPECT_FALSE(IsSqlReservedWord("MOD"));
  EXPECT_FALSE(IsSqlReservedWord("MOD_BY"));
  EXPECT_FALSE(IsSqlReservedWord("STORAGE"));
  EXPECT_FALSE(IsSqlReservedWord("M_SIZE"));

  EXPECT_FALSE(IsSqlReservedWord("TYPE_SIZE"));
  EXPECT_FALSE(IsSqlReservedWord("INT_POL"));
  EXPECT_FALSE(IsSqlReservedWord("MIN_VAL"));
  EXPECT_FALSE(IsSqlReservedWord("MAX_VAL"));
  EXPECT_FALSE(IsSqlReservedWord("AVG_VAL"));
  EXPECT_FALSE(IsSqlReservedWord("DEV_VAL"));
  EXPECT_FALSE(IsSqlReservedWord("QUANTITY"));
  EXPECT_FALSE(IsSqlReservedWord("UNIT"));
  EXPECT_FALSE(IsSqlReservedWord("CHANNEL"));
  EXPECT_FALSE(IsSqlReservedWord("MEAS"));

  EXPECT_FALSE(IsSqlReservedWord("NOF_ROWS"));

  EXPECT_FALSE(IsSqlReservedWord("G_FLAG"));
  EXPECT_FALSE(IsSqlReservedWord("IND"));
  EXPECT_FALSE(IsSqlReservedWord("SEQ"));
  EXPECT_FALSE(IsSqlReservedWord("DATA_TYPE"));
  EXPECT_FALSE(IsSqlReservedWord("VAL_BLOB"));
  EXPECT_FALSE(IsSqlReservedWord("SUB_MATRIX"));

  EXPECT_FALSE(IsSqlReservedWord("ORDINAL"));
  EXPECT_FALSE(IsSqlReservedWord("COMP_SIZE"));
  EXPECT_FALSE(IsSqlReservedWord("FILENAME"));
  EXPECT_FALSE(IsSqlReservedWord("VAL_TYPE"));
  EXPECT_FALSE(IsSqlReservedWord("START_OFF"));
  EXPECT_FALSE(IsSqlReservedWord("BLOCK_SIZE"));
  EXPECT_FALSE(IsSqlReservedWord("VAL_BLOCK"));
  EXPECT_FALSE(IsSqlReservedWord("VAL_OFF"));
  EXPECT_FALSE(IsSqlReservedWord("FLAG_FILE"));
  EXPECT_FALSE(IsSqlReservedWord("FLAG_OFF"));
  EXPECT_FALSE(IsSqlReservedWord("BIT_COUNT"));
  EXPECT_FALSE(IsSqlReservedWord("BIT_OFF"));
  EXPECT_FALSE(IsSqlReservedWord("LOC_COL"));
  EXPECT_FALSE(IsSqlReservedWord("VALS_FILET"));
  EXPECT_FALSE(IsSqlReservedWord("FLAGS_FILE"));

  EXPECT_FALSE(IsSqlReservedWord("TEST"));
  EXPECT_FALSE(IsSqlReservedWord("UUT"));
  EXPECT_FALSE(IsSqlReservedWord("UUTP"));
  EXPECT_FALSE(IsSqlReservedWord("TEST_SEQ"));
  EXPECT_FALSE(IsSqlReservedWord("TEST_SEQP"));
  EXPECT_FALSE(IsSqlReservedWord("TEST_EQ"));
  EXPECT_FALSE(IsSqlReservedWord("TEST_EQP"));
  EXPECT_FALSE(IsSqlReservedWord("PASSWORD"));
  EXPECT_FALSE(IsSqlReservedWord("DISABLED"));
  EXPECT_FALSE(IsSqlReservedWord("PARENT"));
  EXPECT_FALSE(IsSqlReservedWord("LOG_TIME"));

  EXPECT_FALSE(IsSqlReservedWord("PAR_VAL"));
  EXPECT_FALSE(IsSqlReservedWord("UNIT"));
  EXPECT_FALSE(IsSqlReservedWord("PAR_SET"));

}

}

