if (BUILD_EXAMPLES)

    set(EXAMPLE_ADVANCED_CLIENT_SRC
            "${EXAMPLE_DIR}/advanced/advanced_client.cpp")

    message("Set target: advanced_client")
    add_executable(advanced_client "${EXAMPLE_ADVANCED_CLIENT_SRC}")
    target_compile_options(advanced_client PRIVATE ${COMPILER_FLAGS})
    set_target_properties(advanced_client PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
    set_definitions(advanced_client "")
    if (SUPPORT_SSL)
        target_link_libraries(advanced_client ${OPENSSL_LIBRARIES})
    endif()
    if (BUILD_INTERNAL_SHARED)
        target_link_libraries(advanced_client ${LINK_LIBRARIES} radrpc_shared)
    else()
        target_link_libraries(advanced_client ${LINK_LIBRARIES} radrpc_static)
    endif()

endif()