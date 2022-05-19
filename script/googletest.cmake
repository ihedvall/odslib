# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT

if (NOT GTest_FOUND)
    set(GTEST_MSVC_SEARCH MT)

    find_package(GTest)
    message(STATUS "GTest Found: " ${GTEST})
    message(STATUS "GTest Include Dirs: " ${GTEST_INCLUDE_DIRS})
    message(STATUS "GTest Libraries: " ${GTEST_LIBRARIES})
    message(STATUS "GTest Main Libraries: " ${GTEST_MAIN_LIBRARIES})
    message(STATUS "GTest Both Libraries: " ${GTEST_BOTH_LIBRARIES})
    if (NOT GTest_FOUND)
        set(GTEST_ROOT "k:/googletest/master")
        find_package(GTest)
        message(STATUS "GTest Found: " ${GTEST})
        message(STATUS "GTest Include Dirs: " ${GTEST_INCLUDE_DIRS})
        message(STATUS "GTest Libraries: " ${GTEST_LIBRARIES})
        message(STATUS "GTest Main Libraries: " ${GTEST_MAIN_LIBRARIES})
        message(STATUS "GTest Both Libraries: " ${GTEST_BOTH_LIBRARIES})
    endif()
endif()