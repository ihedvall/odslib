# Copyright 2023 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(event_log_rpc
        VERSION 1.0
        DESCRIPTION "Event Log RPC Server executable (service/daemon)"
        LANGUAGES CXX)

add_executable(event_log_rpc src/eventlogrpcd.cpp)
set_target_properties(event_log_rpc PROPERTIES OUTPUT_NAME "eventlogrpcd")

target_include_directories(event_log_rpc PRIVATE ../include)
target_include_directories(event_log_rpc PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(event_log_rpc PRIVATE ${utillib_SOURCE_DIR}/include)
target_include_directories(event_log_rpc PRIVATE ${workflowlib_SOURCE_DIR}/include)


target_compile_definitions(event_log_rpc PRIVATE XML_STATIC)

if (MINGW)
    target_link_options(event_log_rpc PRIVATE -static -fstack-protector)
elseif (MSVC)
    target_compile_definitions(event_log_rpc PRIVATE _WIN32_WINNT=0x0A00)
endif ()

target_link_libraries(event_log_rpc PRIVATE ods)
target_link_libraries(event_log_rpc PRIVATE workflow)
target_link_libraries(event_log_rpc PRIVATE mdf)
target_link_libraries(event_log_rpc PRIVATE util)
target_link_libraries(event_log_rpc PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(event_log_rpc PRIVATE ${Boost_LIBRARIES})
target_link_libraries(event_log_rpc PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(event_log_rpc PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpq.lib)
target_link_libraries(event_log_rpc PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgtypes.lib)
target_link_libraries(event_log_rpc PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgport.lib)
target_link_libraries(event_log_rpc PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgcommon.lib)

target_link_libraries(event_log_rpc PRIVATE absl::log)
target_link_libraries(event_log_rpc PRIVATE absl::log_internal_check_op)
target_link_libraries(event_log_rpc PRIVATE utf8_range::utf8_range)
target_link_libraries(event_log_rpc PRIVATE utf8_range::utf8_validity)
target_link_libraries(event_log_rpc PRIVATE gRPC::grpc++)
target_link_libraries(event_log_rpc PRIVATE OpenSSL::Crypto)
target_link_libraries(event_log_rpc PRIVATE OpenSSL::SSL)
if (WIN32)
    target_link_libraries(event_log_rpc PRIVATE ws2_32)
    target_link_libraries(event_log_rpc PRIVATE mswsock)
    target_link_libraries(event_log_rpc PRIVATE secur32)
    target_link_libraries(event_log_rpc PRIVATE crypt32)
endif ()
