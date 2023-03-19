# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(TestOds
        VERSION 1.0
        DESCRIPTION "Google unit tests for the ODS library"
        LANGUAGES CXX)

include("../script/zlib.cmake")
include("../script/openssl.cmake")
#include("../script/expat.cmake")

add_executable(test_ods
        testsqlite.h testsqlite.cpp
        testidatabase.cpp
        testodsdef.cpp
        testodsitem.cpp
        testtestdirectory.cpp testlibpq.cpp testpostgres.cpp testpostgres.h)

target_include_directories(test_ods PRIVATE
        ../include
        ../src
        ${utillib_SOURCE_DIR}/include
        ${GTEST_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${SQLite3_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS})

if (MINGW)
    target_link_options(test_ods PRIVATE -static -fstack-protector)
elseif (MSVC)
    target_compile_definitions(test_ods PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(test_ods PRIVATE ods)
target_link_libraries(test_ods PRIVATE mdf)
target_link_libraries(test_ods PRIVATE util)
target_link_libraries(test_ods PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${Boost_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${ZLIB_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${OPENSSL_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${GTEST_BOTH_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpq.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgtypes.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgport.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgcommon.lib)

if (WIN32)
target_link_libraries(test_ods PRIVATE ws2_32)
target_link_libraries(test_ods PRIVATE mswsock)
target_link_libraries(test_ods PRIVATE secur32)
target_link_libraries(test_ods PRIVATE crypt32)
endif()


include(GoogleTest)
gtest_discover_tests(test_ods)

