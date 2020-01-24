set(RADRPC_INCLUDE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

if (SUPPORT_SSL)
    set(RADRPC_INCLUDE
            "${RADRPC_INCLUDE}"
            "${OPENSSL_INCLUDE_DIR}")
endif()

set(RADRPC_INCLUDE
        "${RADRPC_INCLUDE}"
        "${Boost_INCLUDE_DIRS}")

set(RADRPC_TEST_INCLUDE
        "${PROJECT_SOURCE_DIR}"
        "${TEST_DIR}"
        "${TEST_DIR}/dep")