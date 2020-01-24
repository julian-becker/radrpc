if (BUILD_TESTS OR BUILD_STRESS_TESTS OR SUPPORT_COVERAGE)

    set(TEST_SRC      
            "${TEST_DIR}/core/defaults.cpp"
            "${TEST_DIR}/core/test_data.cpp"
            "${TEST_DIR}/core/ssl_context.cpp"
            "${TEST_DIR}/unit/construct/client.cpp"
            "${TEST_DIR}/unit/construct/server.cpp")

    # Build raw on windows
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

        message("Set target: test")

        # Build obj
        add_library(test_obj OBJECT ${TEST_SRC})
        target_include_directories(test_obj PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
        target_compile_options(test_obj PRIVATE ${COMPILER_FLAGS})
        set_target_properties(test_obj PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
        if (BUILD_INTERNAL_SHARED)
            set_property(TARGET test_obj PROPERTY POSITION_INDEPENDENT_CODE 1)
        endif()
        set_definitions(test_obj "")

        if (BUILD_INTERNAL_SHARED)
            # Build shared library
            add_library(test_shared SHARED $<TARGET_OBJECTS:test_obj>)
            set_target_properties(test_shared PROPERTIES OUTPUT_NAME "test")
        else()
            # Build static library
            add_library(test_static STATIC $<TARGET_OBJECTS:test_obj>)
            set_target_properties(test_static PROPERTIES OUTPUT_NAME "test")
        endif()

    # Build instrumted on other platforms
    else()

        # Build instrumted library foreach sanitizer
        if(NOT "${SANTIZERS}" STREQUAL "")
        foreach(SANTIZER ${SANTIZERS})

            message("Set target: test_${SANTIZER}")

            # Add libc++ flag & link instrumented if memory sanitizer is used
            set(COMPILER_FLAGS_ "${COMPILER_FLAGS}")
            if ("${SANTIZER}" STREQUAL "memory" AND INSTRUMENTED_FOUND)
                set(COMPILER_FLAGS_ ${COMPILER_FLAGS_} ${INSTRUMENTED_COMPILER_FLAGS})
            endif()

            # Build obj
            add_library(test_obj_${SANTIZER} OBJECT ${TEST_SRC})
            target_include_directories(test_obj_${SANTIZER} PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
            target_compile_options(test_obj_${SANTIZER} PRIVATE ${COMPILER_FLAGS_} -fsanitize=${SANTIZER})
            set_target_properties(test_obj_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET test_obj_${SANTIZER} PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            set_definitions(test_obj_${SANTIZER} ${SANTIZER})

            if (BUILD_INTERNAL_SHARED)
                # Build shared library
                add_library(test_shared_${SANTIZER} SHARED $<TARGET_OBJECTS:test_obj_${SANTIZER}>)
                set_target_properties(test_shared_${SANTIZER} PROPERTIES OUTPUT_NAME "test_${SANTIZER}")
            else()
                # Build static library
                add_library(test_static_${SANTIZER} STATIC $<TARGET_OBJECTS:test_obj_${SANTIZER}>)
                set_target_properties(test_static_${SANTIZER} PROPERTIES OUTPUT_NAME "test_${SANTIZER}")
            endif()

        endforeach()
        endif()

        # Build raw binary with BUILD_VALGRIND
        if(BUILD_WITH_VALGRIND OR SUPPORT_COVERAGE)

            message("Set target: test_valgrind")

            # Build obj
            add_library(test_valgrind_obj OBJECT ${TEST_SRC})
            target_include_directories(test_valgrind_obj PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
            target_compile_options(test_valgrind_obj PRIVATE ${COMPILER_FLAGS})
            set_target_properties(test_valgrind_obj PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET test_valgrind_obj PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            target_compile_definitions(test_valgrind_obj PRIVATE PRIVATE BUILD_VALGRIND)
            set_definitions(test_valgrind_obj "")

            if (BUILD_INTERNAL_SHARED)
                # Build shared library
                add_library(test_valgrind_shared SHARED $<TARGET_OBJECTS:test_valgrind_obj>)
                set_target_properties(test_valgrind_shared PROPERTIES OUTPUT_NAME "test_valgrind")
            else()
                # Build static library
                add_library(test_valgrind_static STATIC $<TARGET_OBJECTS:test_valgrind_obj>)
                set_target_properties(test_valgrind_static PROPERTIES OUTPUT_NAME "test_valgrind")
            endif()

        endif()

    endif()

endif()