message("Set target: radrpc")

set(RADRPC_SRC         
        "${SRC_DIR}/client.cpp"
        "${SRC_DIR}/server.cpp"
        "${SRC_DIR}/common/receive_buffer.cpp"
        "${SRC_DIR}/impl/server/detect_session.cpp"
        "${SRC_DIR}/impl/server/listener.cpp"
        "${SRC_DIR}/impl/server/session_manager.cpp")

# Build obj
add_library(radrpc_obj OBJECT ${RADRPC_SRC})
target_include_directories(radrpc_obj PRIVATE ${RADRPC_INCLUDE})
target_compile_options(radrpc_obj PRIVATE ${COMPILER_FLAGS})
set_target_properties(radrpc_obj PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
if (BUILD_SHARED)
    set_property(TARGET radrpc_obj PROPERTY POSITION_INDEPENDENT_CODE 1)
endif()
set_definitions(radrpc_obj "")

# Build static library
add_library(radrpc_static STATIC $<TARGET_OBJECTS:radrpc_obj>)
set_target_properties(radrpc_static PROPERTIES OUTPUT_NAME "radrpc")

# Build shared library
if (BUILD_SHARED OR BUILD_INTERNAL_SHARED)
    add_library(radrpc_shared SHARED $<TARGET_OBJECTS:radrpc_obj>)
    set_target_properties(radrpc_shared PROPERTIES OUTPUT_NAME "radrpc")
endif()

# Build instrumted library foreach sanitizer
if(NOT "${SANTIZERS}" STREQUAL "" AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
foreach(SANTIZER ${SANTIZERS})

    message("Set target: radrpc_${SANTIZER}")

    # Add libc++ flag & link instrumented if memory sanitizer is used
    set(COMPILER_FLAGS_ "${COMPILER_FLAGS}")
    set(LINK_LIBRARIES_ "${LINK_LIBRARIES}")
    if ("${SANTIZER}" STREQUAL "memory" AND INSTRUMENTED_FOUND)
        set(COMPILER_FLAGS_ ${COMPILER_FLAGS_} ${INSTRUMENTED_COMPILER_FLAGS})
        set(LINK_LIBRARIES_ ${INSTRUMENTED_LINK_LIBRARIES})
    endif()

    # Build obj
    add_library(radrpc_obj_${SANTIZER} OBJECT ${RADRPC_SRC})
    target_include_directories(radrpc_obj_${SANTIZER} PRIVATE ${RADRPC_INCLUDE})
    target_compile_options(radrpc_obj_${SANTIZER} PRIVATE ${COMPILER_FLAGS_} -fsanitize=${SANTIZER})
    set_target_properties(radrpc_obj_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${LINKER_FLAGS}")
    if (BUILD_SHARED OR BUILD_INTERNAL_SHARED)
        set_property(TARGET radrpc_obj_${SANTIZER} PROPERTY POSITION_INDEPENDENT_CODE 1)
    endif()
    set_definitions(radrpc_obj_${SANTIZER} ${SANTIZER})

    # Build static library
    add_library(radrpc_static_${SANTIZER} STATIC $<TARGET_OBJECTS:radrpc_obj_${SANTIZER}>)
    target_link_libraries(radrpc_static_${SANTIZER} ${LINK_LIBRARIES_})
    set_target_properties(radrpc_static_${SANTIZER} PROPERTIES OUTPUT_NAME "radrpc_${SANTIZER}")

    # Build shared library
    if (BUILD_SHARED OR BUILD_INTERNAL_SHARED)
        add_library(radrpc_shared_${SANTIZER} SHARED $<TARGET_OBJECTS:radrpc_obj_${SANTIZER}>)
        target_link_libraries(radrpc_shared_${SANTIZER} ${LINK_LIBRARIES_})
        set_target_properties(radrpc_shared_${SANTIZER} PROPERTIES OUTPUT_NAME "radrpc_${SANTIZER}")
    endif()

endforeach()
endif()