if (SUPPORT_SSL)
    if (INSTRUMENTED_FOUND)
        include_directories(
                "${LIBCPP_INCLUDE_DIR}")
    endif()
    include_directories(
            "${PROJECT_SOURCE_DIR}"
            "${PROJECT_SOURCE_DIR}/include/"
            "${TEST_DIR}/"
            "${TEST_DIR}/dep/"
            "${OPENSSL_INCLUDE_DIR}"
            "${Boost_INCLUDE_DIRS}")
else()
    include_directories(
            "${PROJECT_SOURCE_DIR}"
            "${PROJECT_SOURCE_DIR}/include/"
            "${TEST_DIR}/"
            "${TEST_DIR}/dep/"
            "${Boost_INCLUDE_DIRS}")
endif()