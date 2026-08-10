# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT
include(CMakePrintHelpers)

if (NOT ZLIB_FOUND)
    SET(ZLIB_USE_STATIC_LIBS ON)
    if (COMP_DIR)
       set(ZLIB_ROOT ${COMP_DIR}/zlib/master)
    endif()
    find_package(ZLIB REQUIRED)
endif()

cmake_print_variables( ZLIB_FOUND
                       ZLIB_VERSION
                       ZLIB_INCLUDE_DIRS
                       ZLIB_LIBRARIES )
cmake_print_properties(TARGETS ZLIB::ZLIB
                       PROPERTIES INTERFACE_INCLUDE_DIRECTORIES LOCATION)

