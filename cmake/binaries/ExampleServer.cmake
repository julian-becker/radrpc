if (BUILD_EXAMPLES)

    set(EXAMPLE_SERVER_SRC
            "${EXAMPLE_DIR}/basic/server.cpp")

    message("Set target: server")
    add_executable(server "${EXAMPLE_SERVER_SRC}")
    target_compile_options(server PRIVATE ${COMPILER_FLAGS})
    set_target_properties(server PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
    set_definitions(server "")
    if (BUILD_INTERNAL_SHARED)
        target_link_libraries(server ${LINK_LIBRARIES} radrpc_shared)
    else()
        target_link_libraries(server ${LINK_LIBRARIES} radrpc_static)
    endif()

endif()