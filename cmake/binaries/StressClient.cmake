if (BUILD_STRESS_TESTS)

    set(STRESS_CLIENT_SRC
            "${TEST_DIR}/stress/stress_client.cpp"
            "${TEST_DIR}/stress/config_file.cpp"
            "${TEST_DIR}/stress/test_suite/client_pool.cpp")



    add_test_executable(stress_client "${STRESS_CLIENT_SRC}" FALSE)

endif()