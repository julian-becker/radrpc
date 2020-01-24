if (BUILD_TESTS OR SUPPORT_COVERAGE)

    set(UNIT_SERVER_SRC
            "${TEST_DIR}/unit/plain_server.cpp")
    if (SUPPORT_SSL)
        set(UNIT_SERVER_SRC "${UNIT_SERVER_SRC}"
            "${TEST_DIR}/unit/ssl_server.cpp")
    endif()

    add_test_executable(unit_server "${UNIT_SERVER_SRC}" TRUE)

endif()