set_option(BUILD_SHARED
        "Builds radrpc also as shared library"
       ON)
set_option(BUILD_INTERNAL_SHARED
        "Build internal libraries as shared objects instead of static"
       OFF)
set_option(BUILD_EXAMPLES
        "Build all examples located in folder 'examples'"
       OFF)
set_option(BUILD_TESTS
        "Build common tests to check the implementation"
       ON)
set_option(BUILD_STRESS_TESTS
        "Build all stress tests"
       ON)
set_option(BIN_ASAN
        "Build targets with AddressSanitizer"
       ON)
set_option(BIN_UBSAN
        "Build targets with UndefinedBehaviorSanitizer"
       OFF)
set_option(BIN_TSAN
        "Build targets with ThreadSanitizer"
       OFF)
set_option(BIN_MSAN
        "Build targets with MemorydSanitizer, this requires instrumented libraries"
       OFF)
set_option(BIN_VALGRIND
        "Build targets configured for Valgrind"
       OFF)
set_option(LOGGING
        "Enables logging for debug purposes"
       ON)
set_option(SUPPORT_COVERAGE
        "Build targets with code coverage, this setting only affects compilers with clang or gnu"
       ON)
set_option(SUPPORT_SSL
        "Configures for ssl usage with OpenSSL"
       ON)
set_option(CREATE_SCRIPTS
        "Create scripts to run the tests, this setting doesn't affect msvc compiler"
       ON)
set(MSAN_LIBCXX_DIR
    "/usr/local/lib/libcxx_msan"
    CACHE STRING
    "Memory sanitizer instrumented libc++ library, only used if BIN_MSAN is set")
set(MSAN_OPENSSL_DIR
    "/usr/local/lib/openssl_1_1_1_msan"
    CACHE STRING
    "Memory sanitizer instrumented OpenSSL library, only used if BIN_MSAN is set")
set(MSAN_BOOST_DIR
    "/usr/local/lib/boost_1_70_0_msan"
    CACHE STRING
    "Memory sanitizer instrumented Boost library, only used if BIN_MSAN is set")

message("${BUILD_EXAMPLES}\tBuild examples")
message("${BUILD_TESTS}\tBuild tests")
message("${BUILD_STRESS_TESTS}\tBuild stress tests")
message("${BIN_ASAN}\tBinaries AddressSanitizer")
message("${BIN_UBSAN}\tBinaries UndefinedBehaviorSanitizer")
message("${BIN_TSAN}\tBinaries ThreadSanitizer")
message("${BIN_VALGRIND}\tBinaries Valgrind")
message("${BIN_RAW}\tBinaries non-instrumented")
message("${LOGGING}\tLogging")
message("${SUPPORT_COVERAGE}\tCoverage support")
message("${SUPPORT_SSL}\tSSL support")
message("${CREATE_SCRIPTS}\tCreate test scripts")