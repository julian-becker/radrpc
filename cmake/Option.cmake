set(CACHE_OPTION ON)

set_option(BUILD_SHARED
        "Builds radrpc also as shared library"
       ON)
set_option(BUILD_INTERNAL_SHARED
        "Build internal libraries as shared objects instead of static"
       OFF)
set_option(BUILD_EXAMPLES
        "Build all examples located in folder 'examples'"
       OFF)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
set(CMAKE_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE STRING
    "Build all targets depending on this type")
set_option(BUILD_TESTS
        "Build common tests to check the implementation"
       OFF)
set_option(BUILD_STRESS_TESTS
        "Build all stress tests"
       OFF)
set_option(BUILD_WITH_ASAN
        "Build test targets with AddressSanitizer"
       OFF)
set_option(BUILD_WITH_UBSAN
        "Build test targets with UndefinedBehaviorSanitizer"
       OFF)
set_option(BUILD_WITH_TSAN
        "Build test targets with ThreadSanitizer"
       OFF)
set_option(BUILD_WITH_MSAN
        "Build test targets with MemorydSanitizer, this requires instrumented libraries"
       OFF)
set_option(BUILD_WITH_VALGRIND
        "Build test targets configured for Valgrind"
       OFF)
set_option(ENABLE_LOGGING
        "Enables logging for debug purposes"
       OFF)
set_option(SUPPORT_COVERAGE
        "Build targets with code coverage, this setting only affects compilers with clang or gnu"
       OFF)
set_option(SUPPORT_SSL
        "Configures for ssl usage with OpenSSL"
       ON)
set(MSAN_LIBCXX_DIR
    "/usr/local/lib/libcxx_msan"
    CACHE STRING
    "Memory sanitizer instrumented libc++ library, only used if BUILD_WITH_MSAN is set")
set(MSAN_OPENSSL_DIR
    "/usr/local/lib/openssl_1_1_1_msan"
    CACHE STRING
    "Memory sanitizer instrumented OpenSSL library, only used if BUILD_WITH_MSAN is set")
set(MSAN_BOOST_DIR
    "/usr/local/lib/boost_1_72_0_msan"
    CACHE STRING
    "Memory sanitizer instrumented Boost library, only used if BUILD_WITH_MSAN is set")

message("${BUILD_EXAMPLES}\tBuild examples")
message("${BUILD_TESTS}\tBuild tests")
message("${BUILD_STRESS_TESTS}\tBuild stress tests")
message("${BUILD_WITH_ASAN}\tBinaries AddressSanitizer")
message("${BUILD_WITH_UBSAN}\tBinaries UndefinedBehaviorSanitizer")
message("${BUILD_WITH_TSAN}\tBinaries ThreadSanitizer")
message("${BUILD_WITH_VALGRIND}\tBinaries Valgrind")
message("${ENABLE_LOGGING}\tLogging")
message("${SUPPORT_COVERAGE}\tCoverage support")
message("${SUPPORT_SSL}\tSSL support")
if (SUPPORT_COVERAGE)
    set("Coverage will be run on *_valgrind specified binaries")
    include(CodeCoverageGNU)
    include(CodeCoverageLLVM)
endif()