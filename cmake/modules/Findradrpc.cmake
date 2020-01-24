set(FIND_RADRPC_PATHS
    ${RADRPC_ROOT}
    $ENV{RADRPC_ROOT}
    ${CMAKE_CURRENT_LIST_DIR}/..                   # To support in-tree build
    ${CMAKE_CURRENT_LIST_DIR}/../build/output/lib  #
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /opt/local
    /opt)

find_path(RADRPC_INCLUDE_DIR
    PATH_SUFFIXES "include"
    NAMES "radrpc/version.h"
    PATHS ${FIND_RADRPC_PATHS})

find_library(RADRPC_LIBRARIES
    NAMES librpc radrpc
    PATHS ${FIND_RADRPC_PATHS})

if(RADRPC_INCLUDE_DIR)
    file(READ
        "${RADRPC_INCLUDE_DIR}/radrpc/version.h"
        RADRPC_VERSION_CONTENTS)
    string(REGEX REPLACE
        ".*#define RADRPC_VERSION_MAJOR ([0-9]+).*" "\\1"
        RADRPC_VERSION_MAJOR "${RADRPC_VERSION_CONTENTS}")
    string(REGEX REPLACE
        ".*#define RADRPC_VERSION_MINOR ([0-9]+).*" "\\1"
        RADRPC_VERSION_MINOR "${RADRPC_VERSION_CONTENTS}")
    string(REGEX REPLACE
        ".*#define RADRPC_VERSION_PATCH ([0-9]+).*" "\\1"
        RADRPC_VERSION_PATCH "${RADRPC_VERSION_CONTENTS}")
    set(RADRPC_VERSION_STR
        "${RADRPC_VERSION_MAJOR}.${RADRPC_VERSION_MINOR}.${RADRPC_VERSION_PATCH}")
endif()

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(RADRPC_EXTRA_FLAGS "-pthread")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(RADRPC_EXTRA_FLAGS "-pthread")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(RADRPC_EXTRA_FLAGS "/EHsc")
    set(RADRPC_EXTRA_FLAGS_DEBUG "/Zi")
endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    radrpc
    FOUND_VAR RADRPC_FOUND
    REQUIRED_VARS RADRPC_INCLUDE_DIR RADRPC_LIBRARIES
    VERSION_VAR RADRPC_VERSION_STRING)