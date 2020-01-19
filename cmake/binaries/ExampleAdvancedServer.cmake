if (BUILD_EXAMPLES)

    set(EXAMPLE_ADVANCED_SERVER_SRC
            "${EXAMPLE_DIR}/advanced/advanced_server.cpp")

    message("Set target: advanced_server")
    add_executable(advanced_server "${EXAMPLE_ADVANCED_SERVER_SRC}")
    target_include_directories(advanced_server PRIVATE ${RADRPC_INCLUDE})
    target_compile_options(advanced_server PRIVATE ${COMPILER_FLAGS})
    set_target_properties(advanced_server PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
    set_definitions(advanced_server "")
    if (BUILD_INTERNAL_SHARED)
        target_link_libraries(advanced_server ${LINK_LIBRARIES} radrpc_shared)
    else()
        target_link_libraries(advanced_server ${LINK_LIBRARIES} radrpc_static)
    endif()

endif()