# Copyright 2023 Ingemar Hedvall
# SPDX-License-Identifier: MIT
if (NOT absl_FOUND)
    set( absl_ROOT "k:/grpc/master")
    find_package(absl CONFIG)
    message(STATUS "absl Found (Try 1): " ${absl_FOUND})
    cmake_print_properties(TARGETS absl::base PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)
endif()

if (NOT utf8_range_FOUND)
    set( utf8_range_ROOT "k:/grpc/master")
    find_package(utf8_range CONFIG)
    message(STATUS "utf8_range Found (Try 1): " ${utf8_range_FOUND})
    cmake_print_properties(TARGETS utf8_range::utf8_range PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)
endif()

if (NOT gRPC_FOUND)
    set(gRPC_USE_STATIC_LIBS ON)
    # set(gRPC_DEBUG ON)

    find_package(gRPC CONFIG)
    message(STATUS "gRPC Found (Try 1): " ${gRPC_FOUND})
    if (NOT gRPC_FOUND)
        set( gRPC_ROOT "k:/grpc/master")
        find_package(gRPC CONFIG REQUIRED)
        message(STATUS "gRPC Found (Try 2): " ${gRPC_FOUND})

    endif()
endif()

if (gRPC_FOUND)
    get_target_property(gRPC_INCLUDE_DIRS gRPC::grpc INTERFACE_INCLUDE_DIRECTORIES )
    get_target_property(gRPC_LIBRARIES gRPC::grpc INTERFACE_LINK_LIBRARIES )

    find_program(gRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
    cmake_print_properties(TARGETS gRPC::grpc PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)

    cmake_print_properties(TARGETS gRPC::grpc++ PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)
    cmake_print_properties(TARGETS gRPC::grpc++_reflection PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            LOCATION)

    cmake_print_properties(TARGETS gRPC::grpc_cpp_plugin PROPERTIES
            LOCATION)

    cmake_print_variables(gRPC_VERSION)
    cmake_print_variables(gRPC_INCLUDE_DIRS)
    cmake_print_variables(gRPC_LINK_LIBRARIES)
endif()