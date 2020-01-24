if (BUILD_TESTS OR SUPPORT_COVERAGE)

    set(UNIT_MISC_SRC
            "${TEST_DIR}/unit/misc.cpp")





    add_test_executable(unit_misc "${UNIT_MISC_SRC}" TRUE)

endif()