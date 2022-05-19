# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(odslib
        VERSION 1.0
        DESCRIPTION "Interface against ODS databases"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX d)

include("script/boost.cmake")
include("script/sqlite3.cmake")
include ("script/utillib.cmake")
include ("script/mdflib.cmake")

include("script/googletest.cmake")
include("script/doxygen.cmake")

add_library(ods STATIC
        src/itable.cpp include/ods/itable.h
        src/idatabase.cpp include/ods/idatabase.h
        src/databaseguard.cpp include/ods/databaseguard.h
        src/sqlitedatabase.cpp src/sqlitedatabase.h
        src/sqlitestatement.cpp src/sqlitestatement.h
        src/odsdef.cpp include/ods/odsdef.h
        src/imodel.cpp include/ods/imodel.h
        src/icolumn.cpp include/ods/icolumn.h
        src/irelation.cpp include/ods/irelation.h
        src/ienum.cpp include/ods/ienum.h
        src/baseattribute.cpp include/ods/baseattribute.h
        src/atfxfile.cpp include/ods/atfxfile.h
        src/iattribute.cpp include/ods/iattribute.h
        src/iitem.cpp include/ods/iitem.h
        src/ienvironment.cpp include/ods/ienvironment.h
        src/testdirectory.cpp src/testdirectory.h
        src/ienvironmentcreator.cpp include/ods/ienvironmentcreator.h
        src/sqlfilter.cpp include/ods/sqlfilter.h)



target_include_directories(ods PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(ods PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(ods PRIVATE ${utillib_SOURCE_DIR}/include)
target_include_directories(ods PRIVATE ${mdflib_SOURCE_DIR}/include)

target_include_directories(ods PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        )

target_compile_definitions(util PRIVATE XML_STATIC)

set(ODS_PUBLIC_HEADERS include/ods/itable.h)

set_target_properties(ods PROPERTIES PUBLIC_HEADER "${ODS_PUBLIC_HEADERS}")
get_target_property(PH3 ods PUBLIC_HEADER)
message( STATUS "ODS Target Includes: " "${PH3}")

install(TARGETS ods
        EXPORT OdsLibTargets
        LIBRARY DESTINATION ods/lib
        ARCHIVE DESTINATION ods/lib
        RUNTIME DESTINATION ods/bin
        PUBLIC_HEADER DESTINATION ods/include/ods)

if (GTEST_FOUND)
    enable_testing()
    add_subdirectory(test)
endif()

if (DOXYGEN_FOUND AND (CMAKE_BUILD_TYPE MATCHES "^[Rr]elease") )
    set(DOXYGEN_RECURSIVE YES)
    doxygen_add_docs(doc_libods ALL
            include/ods
            COMMENT "ODS Library generating API documentation with Doxygen")
endif()