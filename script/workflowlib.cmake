include (FetchContent)
FetchContent_Declare(workflowlib
        GIT_REPOSITORY https://github.com/ihedvall/workflowlib.git
        GIT_TAG HEAD)

set(WORKFLOW_DOC OFF)
set(WORKFLOW_TOOLS OFF)
set(WORKFLOW_TEST OFF)

FetchContent_MakeAvailable(workflowlib)

message(STATUS "WORKFLOWLIB Populated: " ${workflowlib_POPULATED})
message(STATUS "WORKFLOWLIB Source Dir: " ${workflowlib_SOURCE_DIR})
message(STATUS "WORKFLOWLIB Binary Dir: " ${workflowlib_BINARY_DIR})