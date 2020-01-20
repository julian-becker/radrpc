function(find_llvm_symbolizer)

    if(NOT LLVM_SYMBOLIZER_FOUND)

        set(SEARCH_FILE "llvm*/bin/llvm-symbolizer")

        if ("$ENV{LLVM_ROOT}" STREQUAL "")
            file(GLOB_RECURSE FOUND_FILES
                "/usr/lib64/*${SEARCH_FILE}"
                "/usr/lib/*${SEARCH_FILE}"
                "/usr/local/lib/*${SEARCH_FILE}"
                "/opt/local/lib/*${SEARCH_FILE}"
                "${CMAKE_SOURCE_DIR}/includes/*${SEARCH_FILE}"
                )
        else()
            file(GLOB_RECURSE FOUND_FILES
                "$ENV{LLVM_ROOT}/lib64/*${SEARCH_FILE}"
                "$ENV{LLVM_ROOT}/lib/*${SEARCH_FILE}"
                "${CMAKE_SOURCE_DIR}/lib/*${SEARCH_FILE}"
                )
        endif()

        # Get latest version
        set(LATEST_VERSION 0)
        set(LLVM_SYMBOLIZER_BINARY "")
        foreach(VFILE ${FOUND_FILES})
            get_filename_component(DIR1 "${VFILE}" PATH)
            get_filename_component(DIR2 "${DIR1}" PATH)
            get_filename_component(LLVM_VERSION "${DIR2}" NAME)
            string(REPLACE "llvm-" "" LLVM_VERSION_MAJOR "${LLVM_VERSION}")
            message(STATUS "Found version: ${LLVM_VERSION}")
            if (LLVM_VERSION_MAJOR GREATER LATEST_VERSION)
                set(LATEST_VERSION ${LLVM_VERSION_MAJOR})
                set(LLVM_SYMBOLIZER_BINARY "${VFILE}")
            endif()
        endforeach()

    endif()

    if (LLVM_SYMBOLIZER_BINARY)
        set(LLVM_SYMBOLIZER_FOUND TRUE CACHE INTERNAL "")
    endif()

    if (LLVM_SYMBOLIZER_FOUND)
        set(LLVM_SYMBOLIZER_BINARY "${LLVM_SYMBOLIZER_BINARY}" CACHE INTERNAL "")
        get_filename_component(LLVM_SYMBOLIZER_PATH "${LLVM_SYMBOLIZER_BINARY}" DIRECTORY)
        set(LLVM_SYMBOLIZER_PATH "${LLVM_SYMBOLIZER_PATH}" CACHE INTERNAL "")
        message(STATUS "Found llvm symbolizer: ${LLVM_SYMBOLIZER_BINARY}")
    endif()

endfunction()

find_llvm_symbolizer()