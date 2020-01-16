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

    # Build obj
    add_library(radrpc_obj_${SANTIZER} OBJECT ${RADRPC_SRC})
    target_compile_options(radrpc_obj_${SANTIZER} PRIVATE ${COMPILER_FLAGS} -fsanitize=${SANTIZER})
    set_target_properties(radrpc_obj_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${LINKER_FLAGS}")
    if (BUILD_SHARED OR BUILD_INTERNAL_SHARED)
        set_property(TARGET radrpc_obj_${SANTIZER} PROPERTY POSITION_INDEPENDENT_CODE 1)
    endif()
    set_definitions(radrpc_obj_${SANTIZER} ${SANTIZER})

    # Build static library
    add_library(radrpc_static_${SANTIZER} STATIC $<TARGET_OBJECTS:radrpc_obj_${SANTIZER}>)
    set_target_properties(radrpc_static_${SANTIZER} PROPERTIES OUTPUT_NAME "radrpc_${SANTIZER}")

    # Build shared library
    if (BUILD_SHARED OR BUILD_INTERNAL_SHARED)
        add_library(radrpc_shared_${SANTIZER} SHARED $<TARGET_OBJECTS:radrpc_obj_${SANTIZER}>)
        set_target_properties(radrpc_shared_${SANTIZER} PROPERTIES OUTPUT_NAME "radrpc_${SANTIZER}")
    endif()

endforeach()
endif()