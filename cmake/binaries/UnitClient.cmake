if (BUILD_TESTS OR SUPPORT_COVERAGE)

    set(UNIT_CLIENT_SRC
            "${TEST_DIR}/unit/plain_client.cpp")
    if (SUPPORT_SSL)
        set(UNIT_CLIENT_SRC "${UNIT_CLIENT_SRC}"
            "${TEST_DIR}/unit/ssl_client.cpp")
    endif()

    add_test_executable(unit_client "${UNIT_CLIENT_SRC}" TRUE)

endif()