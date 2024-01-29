# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(TestOds
        VERSION 1.0
        DESCRIPTION "Google unit tests for the ODS library"
        LANGUAGES CXX)

include("../script/zlib.cmake")


add_executable(test_ods
        testsqlite.h testsqlite.cpp
        testidatabase.cpp
        testodsdef.cpp
        testodsitem.cpp
        testtestdirectory.cpp testlibpq.cpp 
		testpostgres.cpp testpostgres.h 
		testsyslogrunner.cpp testsyslogrunner.h)

target_include_directories(test_ods PRIVATE
        ../include
        ../src
        ${utillib_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${SQLite3_INCLUDE_DIRS}
        ${PostgreSQL_INCLUDE_DIRS})

target_include_directories(test_ods PRIVATE ${workflowlib_SOURCE_DIR}/include)
target_include_directories(test_ods PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
if (MINGW)
    target_link_options(test_ods PRIVATE -static -fstack-protector)
elseif (MSVC)
    target_compile_definitions(test_ods PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(test_ods PRIVATE ods)
target_link_libraries(test_ods PRIVATE workflow)
target_link_libraries(test_ods PRIVATE mdf)
target_link_libraries(test_ods PRIVATE util)
target_link_libraries(test_ods PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${Boost_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${ZLIB_LIBRARIES})
target_link_libraries(test_ods PRIVATE GTest::gtest_main)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpq.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgtypes.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgport.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgcommon.lib)
target_link_libraries(test_ods PRIVATE ${OPENSSL_LIBRARIES})
target_link_libraries(test_ods PRIVATE absl::log)
target_link_libraries(test_ods PRIVATE absl::log_internal_check_op)
target_link_libraries(test_ods PRIVATE utf8_range::utf8_range)
target_link_libraries(test_ods PRIVATE utf8_range::utf8_validity)
target_link_libraries(test_ods PRIVATE gRPC::grpc++)

if (WIN32)
target_link_libraries(test_ods PRIVATE ws2_32)
target_link_libraries(test_ods PRIVATE mswsock)
target_link_libraries(test_ods PRIVATE secur32)
target_link_libraries(test_ods PRIVATE crypt32)
endif()


include(GoogleTest)
gtest_discover_tests(test_ods)

