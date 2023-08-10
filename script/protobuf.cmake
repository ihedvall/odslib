# Copyright 2023 Ingemar Hedvall
# SPDX-License-Identifier: MIT

if (NOT Protobuf_FOUND)
    set(Protobuf_USE_STATIC_LIBS ON)
    set(Protobuf_DEBUG OFF)
    find_package(Protobuf)
    message(STATUS "Protobuf Found (Try 1): " ${Protobuf_FOUND})

    if (NOT Protobuf_FOUND)
        set(Protobuf_ROOT "K:/grpc")
        find_package(Protobuf REQUIRED)
        message(STATUS "Protobuf Found (Try 2): " ${Protobuf_FOUND})
    endif()
endif()

if (Protobuf_FOUND)
    cmake_print_variables(Protobuf_VERSION)
    cmake_print_variables(Protobuf_INCLUDE_DIRS)
    cmake_print_variables(Protobuf_LIBRARIES)
    cmake_print_variables(Protobuf_PROTOC_LIBRARIES)
    cmake_print_variables(Protobuf_LITE_LIBRARIES)
    cmake_print_variables(Protobuf_LIBRARY)
    cmake_print_variables(Protobuf_PROTOC_LIBRARY)
    cmake_print_variables(Protobuf_LITE_LIBRARY)
    cmake_print_variables(Protobuf_PROTOC_EXECUTABLE)

    cmake_print_properties(TARGETS protobuf::libprotobuf PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)
    cmake_print_properties(TARGETS protobuf::libprotobuf-lite PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)
    cmake_print_properties(TARGETS protobuf::libprotoc PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)
endif()