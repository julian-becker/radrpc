set(RADRPC_INCLUDE
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/include/")

if (SUPPORT_SSL)
    set(RADRPC_INCLUDE
            "${RADRPC_INCLUDE}"
            "${OPENSSL_INCLUDE_DIR}")
endif()

set(RADRPC_INCLUDE
        "${RADRPC_INCLUDE}"
        "${Boost_INCLUDE_DIRS}")

set(RADRPC_TEST_INCLUDE
        "${TEST_DIR}/"
        "${TEST_DIR}/dep/")