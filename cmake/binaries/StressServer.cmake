if (BUILD_STRESS_TESTS)

    set(STRESS_SERVER_SRC
            "${TEST_DIR}/stress/stress_server.cpp")





    add_test_executable(stress_server "${STRESS_SERVER_SRC}" FALSE)

endif()