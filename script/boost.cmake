# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT

include(CMakePrintHelpers)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

if (NOT Boost_FOUND)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_ARCHITECTURE -x64)
    set(Boost_NO_WARN_NEW_VERSIONS ON)
    set(Boost_DEBUG OFF)

    find_package(Boost COMPONENTS process filesystem system locale program_options)

    if (NOT Boost_FOUND)
        set(Boost_ROOT ${COMP_DIR}/boost/latest)
        find_package(Boost REQUIRED COMPONENTS process filesystem system locale program_options)

    endif()
endif()

cmake_print_variables(Boost_FOUND
                      Boost_VERSION_STRING
                      Boost_INCLUDE_DIRS
                      Boost_LIBRARY_DIRS
                      Boost_LIBRARIES )

cmake_print_properties(TARGETS Boost::headers Boost::filesystem
        PROPERTIES LOCATION INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LIBRARY_DIRECTORIES )