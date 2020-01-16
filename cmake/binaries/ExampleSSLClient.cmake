if (BUILD_EXAMPLES AND SUPPORT_SSL)

    set(EXAMPLE_SSL_CLIENT_SRC
            "${EXAMPLE_DIR}/ssl/ssl_client.cpp")

    message("Set target: ssl_client")
    add_executable(ssl_client "${EXAMPLE_SSL_CLIENT_SRC}")
    target_compile_options(ssl_client PRIVATE ${COMPILER_FLAGS})
    set_target_properties(ssl_client PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
    set_definitions(ssl_client "")
    if (SUPPORT_SSL)
        target_link_libraries(ssl_client ${OPENSSL_LIBRARIES})
    endif()
    if (BUILD_INTERNAL_SHARED)
        target_link_libraries(ssl_client ${LINK_LIBRARIES} radrpc_shared)
    else()
        target_link_libraries(ssl_client ${LINK_LIBRARIES} radrpc_static)
    endif()

endif()