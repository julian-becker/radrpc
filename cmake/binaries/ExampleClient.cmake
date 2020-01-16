if (BUILD_EXAMPLES)

    set(EXAMPLE_CLIENT_SRC
            "${EXAMPLE_DIR}/basic/client.cpp")

    message("Set target: client")
    add_executable(client "${EXAMPLE_CLIENT_SRC}")
    target_compile_options(client PRIVATE ${COMPILER_FLAGS})
    set_target_properties(client PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
    set_definitions(client "")
    if (SUPPORT_SSL)
        target_link_libraries(client ${OPENSSL_LIBRARIES})
    endif()
    if (BUILD_INTERNAL_SHARED)
        target_link_libraries(client ${LINK_LIBRARIES} radrpc_shared)
    else()
        target_link_libraries(client ${LINK_LIBRARIES} radrpc_static)
    endif()

endif()