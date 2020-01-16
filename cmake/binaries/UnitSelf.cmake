if (BUILD_TESTS)

    set(UNIT_SELF_SRC
            "${TEST_DIR}/unit/self.cpp")





    add_test_executable(unit_self "${UNIT_SELF_SRC}" TRUE)

endif()