# https://chromium.googlesource.com/chromium/llvm-project/libcxx/+/refs/heads/master/cmake/Modules/CodeCoverage.cmake
#
#
#
#                       THIS IS A MODIFIED VERSION!
#
#
#

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")

find_program(
    CODE_COVERAGE_LLVM_COV 
    NAMES 
        llvm-cov 
        llvm-cov-11
        llvm-cov-10
        llvm-cov-9
        llvm-cov-8
        llvm-cov-7
        llvm-cov-6
        llvm-cov-5)
if (NOT CODE_COVERAGE_LLVM_COV)
  message(FATAL_ERROR "Cannot find llvm-cov...")
endif()

find_program(
    CODE_COVERAGE_LLVM_PROFDATA
    NAMES 
        llvm-profdata 
        llvm-profdata-11
        llvm-profdata-10
        llvm-profdata-9
        llvm-profdata-8
        llvm-profdata-7
        llvm-profdata-6
        llvm-profdata-5)
if (NOT CODE_COVERAGE_LLVM_PROFDATA)
  message(FATAL_ERROR "Cannot find llvm-profdata...")
endif()

find_program(CODE_COVERAGE_GENHTML genhtml)
if (NOT CODE_COVERAGE_GENHTML)
  message(FATAL_ERROR "Cannot find genhtml...")
endif()

set(CMAKE_CXX_FLAGS_COVERAGE "-fprofile-instr-generate -fcoverage-mapping")

function(clang_setup_target_for_coverage_llvmcov)

    set(options NO_DEMANGLE)
    set(oneValueArgs CAPTURE_DIRECTORY NAME)
    set(multiValueArgs EXCLUDE EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(OUTPUT_DIR "Coverage")

    if ("${Coverage_DEPENDENCIES}" STREQUAL "")
        message(FATAL "No dependencies are defined")
        return()
    endif()
    list(GET Coverage_DEPENDENCIES 0 REF_EXECUTABLE)
    
    # Setup target
    add_custom_target(${Coverage_NAME}

        # Generate profraw
        COMMAND ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}

        # Merge coverage data (here it is just a single file)
        COMMAND ${CODE_COVERAGE_LLVM_PROFDATA} merge -sparse default.profraw -o default.profdata

        # Generate HTML output
        COMMAND ${CODE_COVERAGE_LLVM_COV} show -output-dir=${OUTPUT_DIR} -show-line-counts-or-regions -Xdemangler c++filt -format="html" -instr-profile=default.profdata ${REF_EXECUTABLE} ${Coverage_CAPTURE_DIRECTORY}

        # Set output files as GENERATED (will be removed on 'make clean')
        BYPRODUCTS
            default.profraw
            default.profdata
            ${Coverage_NAME}  # report directory

        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
        COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
    )

endfunction()

function(clang_append_coverage_compiler_flags)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}" PARENT_SCOPE)
    message(STATUS "Appending code coverage compiler flags: ${CMAKE_CXX_FLAGS_COVERAGE}")

endfunction()

endif()