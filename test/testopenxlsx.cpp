/*
 * Copyright 2024 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <OpenXLSX.hpp>

namespace ods::test {

using namespace OpenXLSX;
using namespace std::filesystem;

TEST(OpenXLSX, TestBasic) {
  constexpr std::string_view test_file = "k:/test/import/VCC_ParameterName.xlsx";
  try {
    path filename(test_file);
    if (!exists(filename)) {
      GTEST_SKIP_("Test file missing");
    }

    XLDocument doc;
    doc.open(test_file.data());
    EXPECT_TRUE(doc.isOpen());

    std::cout << "Doc Name: " << doc.name() << std::endl;

    XLWorkbook work_book = doc.workbook();

    const uint16_t nof_sheets = work_book.worksheetCount();
    EXPECT_EQ(nof_sheets, 2);
    for (uint16_t index = 1; index <= nof_sheets; ++index) {
      XLWorksheet sheet = work_book.worksheet(index);
      const XLCellRange range = sheet.range();
      std::cout << "Work Sheet Name: " << index << ": " << sheet.name()
      << ", Range (Columns/Rows): " << range.numColumns() << "/" << range.numRows()
      << std::endl;

      const uint16_t nof_columns = sheet.columnCount();
      for (uint16_t column_index = 1; column_index <= nof_columns; ++column_index) {
        const XLStyleIndex style_index = sheet.getColumnFormat(column_index);
        const XLCellFormat format =  doc.styles().cellFormats().cellFormatByIndex(style_index);
        const XLCellStyle style =  doc.styles().cellStyles().cellStyleByIndex(style_index);
        const XLCellAssignable cell = sheet.cell(1, column_index);
        const XLColumn column = sheet.column(column_index);
        const XLNumberFormats number_formats = doc.styles().numberFormats();
        std::cout << "Column: " << cell.value().getString() << std::endl;
      }

    }

  } catch (const std::exception& err) {
    FAIL() << err.what();
  }
}

}