if (BUILD_EXAMPLES AND SUPPORT_SSL)

    set(EXAMPLE_SSL_SERVER_SRC
            "${EXAMPLE_DIR}/ssl/ssl_server.cpp")

    message("Set target: ssl_server")
    add_executable(ssl_server "${EXAMPLE_SSL_SERVER_SRC}")
    target_compile_options(ssl_server PRIVATE ${COMPILER_FLAGS})
    set_target_properties(ssl_server PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
    set_definitions(ssl_server "")
    if (BUILD_INTERNAL_SHARED)
        target_link_libraries(ssl_server ${LINK_LIBRARIES} radrpc_shared)
    else()
        target_link_libraries(ssl_server ${LINK_LIBRARIES} radrpc_static)
    endif()

endif()