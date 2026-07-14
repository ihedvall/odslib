# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(TestOds
        VERSION 1.0
        DESCRIPTION "Google unit tests for the ODS library"
        LANGUAGES CXX)

include("../script/zlib.cmake")

if (ODS_POSTGRES)
    set(POSTGRES_TEST_FILES testlibpq.cpp testpostgres.cpp testpostgres.h )
else()
    set(POSTGRES_TEST_FILES )
endif ()

add_executable(test_ods
        testsqlite.h testsqlite.cpp
        testidatabase.cpp
        testodsdef.cpp
        testodsitem.cpp
        testtestdirectory.cpp
        ${POSTGRES_TEST_FILES}
		testsyslogrunner.cpp testsyslogrunner.h
        testmodel.cpp
        testmodel.h
        testdatabase.cpp
        testdatabase.h
        testodshelper.cpp
        testattribute.cpp
)

target_include_directories(test_ods PRIVATE
        ../include
        ../src
        ${utillib_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${SQLite3_INCLUDE_DIRS})
if (ODS_POSTGRES)
    target_include_directories(test_ods PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    target_compiler_definition(test_ods PRIVATE ODS_POSTGRES)
endif()

target_include_directories(test_ods PRIVATE ${workflowlib_SOURCE_DIR}/include)
target_include_directories(test_ods PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
target_include_directories(test_ods PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern/sqlite/src)

if (MINGW)
    target_link_options(test_ods PRIVATE -static -fstack-protector)
elseif (MSVC)
    target_compile_definitions(test_ods PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(test_ods PRIVATE ods)
target_link_libraries(test_ods PRIVATE workflow)
target_link_libraries(test_ods PRIVATE mdf)
target_link_libraries(test_ods PRIVATE util)
target_link_libraries(test_ods PRIVATE sago::platform_folders)

target_link_libraries(test_ods PRIVATE ${Boost_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(test_ods PRIVATE ${ZLIB_LIBRARIES})

target_link_libraries(test_ods PRIVATE GTest::gtest_main)

if (ODS_POSTGRES)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpq.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgtypes.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgport.lib)
target_link_libraries(test_ods PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgcommon.lib)
target_link_libraries(test_ods PRIVATE ${OPENSSL_LIBRARIES})
endif()

if (WIN32)
target_link_libraries(test_ods PRIVATE ws2_32)
target_link_libraries(test_ods PRIVATE mswsock)
target_link_libraries(test_ods PRIVATE secur32)
target_link_libraries(test_ods PRIVATE crypt32)
endif()

if (ODS_POSTGRES)
foreach (pqfile ${PostgreSQL_DEPENDENCIES})
    add_custom_command(
            TARGET test_ods
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ARGS ${pqfile} "${CMAKE_CURRENT_BINARY_DIR}"
    )
endforeach ()
endif()

include(GoogleTest)
gtest_discover_tests(test_ods)

