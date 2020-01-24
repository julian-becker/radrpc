
if (SUPPORT_COVERAGE)

    if (BUILD_INTERNAL_SHARED)
        message(FATAL 
            "Coverage with shared library objects is not possilbe, 
            please set BUILD_INTERNAL_SHARED to OFF")
        return()
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

        # Currently llvm-cov doesn't support
        # multi binary report generation, only
        # multiple data can be merged.
        # Maybe it would be possible to
        # compile as separate objects files and link together
        # as one executable, but since the gnu way is
        # working better in this case, it's ok now.
        clang_setup_target_for_coverage_llvmcov(
            NAME
                coverage
            CAPTURE_DIRECTORY 
                "${PROJECT_SOURCE_DIR}/include/"
            EXECUTABLE
                ctest -j 1
            DEPENDENCIES
                unit_client_valgrind
                unit_misc_valgrind
                unit_self_valgrind
                unit_server_valgrind)

    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

        set(COVERAGE_EXCLUDES
            "/usr/*"
            "boost*"
            "boost/*"
            "test/*"
            "boost/*"
            "${PROJECT_SOURCE_DIR}/test/core/*"
            "${PROJECT_SOURCE_DIR}/test/dep/*"
            "${PROJECT_SOURCE_DIR}/test/unit/*"
            "${PROJECT_SOURCE_DIR}/test/unit/construct/*")

        gnu_setup_target_for_coverage_lcov(
            NAME
                coverage
            LCOV_ARGS 
                "--no-external"
            EXECUTABLE
                ctest -j 1
            DEPENDENCIES
                unit_client_valgrind
                unit_misc_valgrind
                unit_self_valgrind
                unit_server_valgrind)

    endif()

endif()