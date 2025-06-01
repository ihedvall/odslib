# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(GlobalName
        VERSION 1.0
        DESCRIPTION "Global Name Configuration Tool"
        LANGUAGES CXX)

add_executable(global_name
        src/globalnameapp.cpp src/globalnameapp.h
        src/globalnameid.h
        src/globalnamedoc.cpp
        src/globalnamedoc.h
        src/globalnameview.cpp
        src/globalnameview.h
        src/mainframe.cpp
        src/mainframe.h
        src/globalname.rc)



target_include_directories(global_name PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_include_directories(global_name  PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(global_name PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../extern/sqlite/src)
target_include_directories(global_name PRIVATE  ${PostgreSQL_INCLUDE_DIRS})
target_include_directories(global_name PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")


target_compile_definitions(util PRIVATE XML_STATIC)

if (MINGW)
    target_link_options(global_name PRIVATE -mwindows -static -fstack-protector)
endif()

if (MSVC)
    target_link_options(global_name PRIVATE /SUBSYSTEM:WINDOWS)
    target_compile_definitions(global_name PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(global_name PRIVATE ods)
target_link_libraries(global_name PRIVATE util)
#target_link_libraries(global_name PRIVATE workflow)
target_link_libraries(global_name PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(global_name PRIVATE ${wxWidgets_LIBRARIES})
target_link_libraries(global_name PRIVATE ${Boost_LIBRARIES})
target_link_libraries(global_name PRIVATE ${ZLIB_LIBRARIES})

target_link_libraries(global_name PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpq.lib)
target_link_libraries(global_name PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgtypes.lib)
target_link_libraries(global_name PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgport.lib)
target_link_libraries(global_name PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgcommon.lib)


if (WIN32)
#target_link_libraries(odsconfig PRIVATE iconv )
target_link_libraries(global_name PRIVATE bcrypt)
target_link_libraries(global_name PRIVATE ws2_32)
target_link_libraries(global_name PRIVATE secur32)
target_link_libraries(global_name PRIVATE crypt32)

endif()

foreach (pqfile ${PostgreSQL_DEPENDENCIES})

#Copy the Postgres DLL/SO files to the current binary directory
add_custom_command(
        TARGET global_name
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ARGS ${pqfile} "${CMAKE_CURRENT_BINARY_DIR}"
)
endforeach ()
