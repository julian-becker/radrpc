if (BUILD_TESTS OR BUILD_STRESS_TESTS OR SUPPORT_COVERAGE)

    include(Dart)
    enable_testing()
    set(TEST_ENVIRONMENT
        "ASAN_OPTIONS=symbolize=1:detect_leaks=1:abort_on_error=1"
        "TSAN_OPTIONS=external_symbolizer_path=${LLVM_SYMBOLIZER_BINARY}:report_signal_unsafe=0"
        "UBSAN_OPTIONS=print_stacktrace=1"
        "ASAN_SYMBOLIZER_PATH=${LLVM_SYMBOLIZER_BINARY}"
        "MSAN_SYMBOLIZER_PATH=${LLVM_SYMBOLIZER_BINARY}"
        "PATH=$ENV{PATH}:${LLVM_SYMBOLIZER_PATH}")
    set(VALGRIND_ARGS 
        "--error-exitcode=8"
        "--error-limit=no"
        "--quiet"
        "--leak-check=full"
        "--track-origins=yes"
        "--track-origins=yes"
        "--vgdb=no")

endif()