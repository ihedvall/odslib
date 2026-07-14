include (FetchContent)
Include (CMakePrintHelpers)

FetchContent_Declare(utillib
        GIT_REPOSITORY https://github.com/ihedvall/utillib.git
        GIT_TAG HEAD)

set(UTIL_DOC OFF)
set(UTIL_TOOLS OFF)
set(UTIL_TEST OFF)
set(UTIL_LEX OFF)

FetchContent_MakeAvailable(utillib)

cmake_print_properties(TARGETS util sago::platform_folders lfreist-hwinfo::hwinfo PROPERTIES INCLUDE_DIRECTORIES)
cmake_print_variables( utillib_POPULATED utillib_SOURCE_DIR utillib_BINARY_DIR)