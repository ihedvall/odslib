# Copyright 2022 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(OdsConfigurator
        VERSION 1.0
        DESCRIPTION "ODS Database Configurator"
        LANGUAGES CXX)

add_executable(odsconfig 
        src/odsconfig.cpp src/odsconfig.h
        src/mainframe.cpp src/mainframe.h
        src/odsdocument.cpp src/odsdocument.h
        src/odsview.cpp src/odsview.h
        src/odsconfig.rc
        src/childframe.cpp src/childframe.h
        src/odsconfigid.h
        src/commonpanel.cpp src/commonpanel.h
        src/enumpanel.cpp src/enumpanel.h
        src/tablepanel.cpp src/tablepanel.h
        src/tabledialog.cpp src/tabledialog.h
        src/columndialog.cpp src/columndialog.h
        src/databasenamevalidator.cpp src/databasenamevalidator.h
        src/enumdialog.cpp src/enumdialog.h
        src/enumitemdialog.cpp src/enumitemdialog.h
        src/appnamevalidator.cpp src/appnamevalidator.h
        src/postgresdialog.cpp src/postgresdialog.h)


target_include_directories(odsconfig PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_include_directories(odsconfig  PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(odsconfig PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(odsconfig PRIVATE  ${PostgreSQL_INCLUDE_DIRS})
target_include_directories(odsconfig PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")


target_compile_definitions(util PRIVATE XML_STATIC)

if (MINGW)
    target_link_options(odsconfig PRIVATE -mwindows -static -fstack-protector)
endif()

if (MSVC)
    target_link_options(odsconfig PRIVATE /SUBSYSTEM:WINDOWS)
    target_compile_definitions(odsconfig PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(odsconfig PRIVATE ods)
target_link_libraries(odsconfig PRIVATE util)
target_link_libraries(odsconfig PRIVATE workflow)
target_link_libraries(odsconfig PRIVATE ${EXPAT_LIBRARIES})
target_link_libraries(odsconfig PRIVATE ${wxWidgets_LIBRARIES})
target_link_libraries(odsconfig PRIVATE ${Boost_LIBRARIES})
target_link_libraries(odsconfig PRIVATE ${ZLIB_LIBRARIES})
target_link_libraries(odsconfig PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(odsconfig PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpq.lib)
target_link_libraries(odsconfig PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgtypes.lib)
target_link_libraries(odsconfig PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgport.lib)
target_link_libraries(odsconfig PRIVATE ${PostgreSQL_LIBRARY_DIRS}/libpgcommon.lib)


if (WIN32)
#target_link_libraries(odsconfig PRIVATE iconv )
target_link_libraries(odsconfig PRIVATE bcrypt)
target_link_libraries(odsconfig PRIVATE ws2_32)
target_link_libraries(odsconfig PRIVATE secur32)
target_link_libraries(odsconfig PRIVATE crypt32)

endif()

foreach (pqfile ${PostgreSQL_DEPENDENCIES})

#Copy the Postgres DLL/SO files to the current binary directory
add_custom_command(
        TARGET odsconfig
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ARGS ${pqfile} "${CMAKE_CURRENT_BINARY_DIR}"
)
endforeach ()
