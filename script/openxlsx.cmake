# Copyright 2024 Ingemar Hedvall
# SPDX-License-Identifier: MIT

include(CMakePrintHelpers)

set(OpenXLSX_ROOT ${COMP_DIR}/openxlsx/latest)

find_package(OpenXLSX REQUIRED)

set(OpenXLSX_INCLUDE_DIRS ${OpenXLSX_ROOT}/include/OpenXLSX ${OpenXLSX_ROOT}/include/OpenXLSX/headers)

cmake_print_variables(
        OpenXLSX_FOUND
        OpenXLSX_INCLUDE_DIRS)

cmake_print_properties(TARGETS OpenXLSX::OpenXLSX PROPERTIES
    LOCATION
    INTERFACE_INCLUDE_DIRECTORIES
    IMPORTED_LOCATION_RELEASE
    IMPORTED_LOCATION_DEBUG )

