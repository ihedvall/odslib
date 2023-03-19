# Copyright 2022 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(ReportExplorer
        VERSION 1.0
        DESCRIPTION "Report Explorer"
        LANGUAGES CXX)
include("../script/sqlite3.cmake")

add_executable(reportexplorer
        src/reportexplorer.cpp src/reportexplorer.h
        src/mainframe.cpp src/mainframe.h
        src/reportexplorer.rc
        src/reportexplorerid.h
        src/envtab.cpp src/envtab.h
        src/testdirdialog.cpp src/testdirdialog.h
        src/envcreator.cpp src/envcreator.h
        src/measurementtab.cpp src/measurementtab.h
        src/meastreedata.cpp src/meastreedata.h
        src/channelpanel.cpp src/channelpanel.h
        src/measselectionpanel.cpp src/measselectionpanel.h
        src/channellistctrl.cpp src/channellistctrl.h
        src/selectedpanel.cpp src/selectedpanel.h
        src/plottab.cpp src/plottab.h
        src/selectedcomparator.cpp src/selectedcomparator.h
        src/fetchvalue.cpp src/fetchvalue.h)


target_include_directories(reportexplorer PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_include_directories(reportexplorer  PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(reportexplorer PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(reportexplorer PRIVATE ${mdflib_SOURCE_DIR}/include)
target_include_directories(reportexplorer PRIVATE ${utillib_SOURCE_DIR}/include)
target_include_directories(reportexplorer PRIVATE ${odslib_SOURCE_DIR}/include)
target_include_directories(reportexplorer PRIVATE ${odslib_SOURCE_DIR}/src)

target_compile_definitions(reportexplorer PRIVATE XML_STATIC)

if (MINGW)
target_link_options(reportexplorer PRIVATE -mwindows -static -fstack-protector)
endif()

if (MSVC)
    target_link_options(reportexplorer PRIVATE /SUBSYSTEM:WINDOWS)
    target_compile_definitions(reportexplorer PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(reportexplorer PRIVATE ods)
target_link_libraries(reportexplorer PRIVATE mdf)
target_link_libraries(reportexplorer PRIVATE util)
target_link_libraries(reportexplorer PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(reportexplorer PRIVATE ${wxWidgets_LIBRARIES})
target_link_libraries(reportexplorer PRIVATE ${Boost_LIBRARIES})
target_link_libraries(reportexplorer PRIVATE ${ZLIB_LIBRARIES})
target_link_libraries(reportexplorer PRIVATE ${SQLite3_LIBRARIES})

if (MINGW)
target_link_libraries(reportexplorer PRIVATE crypto)
else()
target_link_libraries(reportexplorer PRIVATE ${OPENSSL_CRYPTO_LIBRARY})
endif()

if (WIN32)
#target_link_libraries(reportexplorer PRIVATE iconv )
target_link_libraries(reportexplorer PRIVATE bcrypt)
target_link_libraries(reportexplorer PRIVATE ws2_32)
#target_link_libraries(reportexplorer PRIVATE wsock32)
endif()