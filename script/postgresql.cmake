# Copyright 2023 Ingemar Hedvall
# SPDX-License-Identifier: MIT

if (NOT PostgreSQL_FOUND )
    find_package(PostgreSQL)
endif()

if (NOT PostgreSQL_FOUND)
    if (NOT PostgreSQL_ROOT)
        set(PostgreSQL_ROOT ${COMP_DIR}/PostgreSQL/14)
    endif()

    find_package(PostgreSQL REQUIRED)
endif()

message(STATUS "PostgreSQL Found: " ${PostgreSQL_FOUND})
message(STATUS "PostgreSQL Include Dirs: " ${PostgreSQL_INCLUDE_DIRS})
message(STATUS "PostgreSQL Library Dirs: " ${PostgreSQL_LIBRARY_DIRS})
message(STATUS "PostgreSQL Libraries: " ${PostgreSQL_LIBRARIES})
message(STATUS "PostgreSQL Version: " ${PostgreSQL_VERSION_STRING})
message(STATUS "PostgreSQL Type Include Dir: " ${PostgreSQL_TYPE_INCLUDE_DIR})

