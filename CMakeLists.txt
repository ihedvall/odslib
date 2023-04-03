# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(odslib
        VERSION 1.0
        DESCRIPTION "Interface against ODS databases"
        LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Static libraries are preferred" OFF)
option(BUILD_STATIC_LIBS "Static libraries are normally used" ON)
option(ODS_DOC "If doxygen is installed, then build documentation in Release mode" OFF)
option(ODS_TOOLS "If doxygen is installed, then build documentation in Release mode" ON)
option(ODS_TEST "If doxygen is installed, then build documentation in Release mode" ON)

if (NOT COMP_DIR)
    set(COMP_DIR "k:")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX d)

include ("script/utillib.cmake")
include ("script/mdflib.cmake")
include ("script/workflowlib.cmake")
include("script/boost.cmake")
include("script/sqlite3.cmake")
include("script/postgresql.cmake")
#include("script/zlib.cmake")
include("script/openssl.cmake")
include("script/wxwidgets.cmake")


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
        src/odsfactory.cpp include/ods/odsfactory.h
        src/sqlfilter.cpp include/ods/sqlfilter.h
        src/eventlogdb.cpp src/eventlogdb.h
        src/postgresdb.cpp src/postgresdb.h
        src/postgresstatement.cpp src/postgresstatement.h
        src/sysloginserter.cpp src/sysloginserter.h
        )



target_include_directories(ods PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(ods PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(ods PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_include_directories(ods PRIVATE ${utillib_SOURCE_DIR}/include)
target_include_directories(ods PRIVATE ${mdflib_SOURCE_DIR}/include)
target_include_directories(ods PRIVATE ${workflowlib_SOURCE_DIR}/include)

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

if (wxWidgets_FOUND AND ODS_TOOLS)
    add_subdirectory(odsconfigurator)
    add_subdirectory(reportexplorer)
endif()

if (GTEST_FOUND)
    enable_testing()
    add_subdirectory(test)
endif()

if (DOXYGEN_FOUND AND (CMAKE_BUILD_TYPE MATCHES "^[Rr]elease") )
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_RECURSIVE NO)
    set(DOXYGEN_REPEAT_BRIEF NO)
    set(DOXYGEN_PROJECT_NAME "MDFLib")
    # set(DOXYGEN_HTML_EXTRA_STYLESHEET doxygen/utillib.css)
    # set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/manual)
    doxygen_add_docs(doc_libods ALL
            include/ods
            COMMENT "ODS Library generating API documentation with Doxygen")
endif()