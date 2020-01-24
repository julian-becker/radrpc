if (BUILD_TESTS OR BUILD_STRESS_TESTS OR SUPPORT_COVERAGE)

    set(CATCH_MAIN_SRC      
            "${TEST_DIR}/unit/catch_main.cpp")

    # Build raw on windows
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

        message("Set target: catch_main")

        # Build obj
        add_library(catch_main_obj OBJECT ${CATCH_MAIN_SRC})
        target_include_directories(catch_main_obj PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
        target_compile_options(catch_main_obj PRIVATE ${COMPILER_FLAGS})
        set_target_properties(catch_main_obj PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
        if (BUILD_INTERNAL_SHARED)
            set_property(TARGET catch_main_obj PROPERTY POSITION_INDEPENDENT_CODE 1)
        endif()
        set_definitions(catch_main_obj "")

    # Build instrumted on other platforms
    else()

        # Build instrumted library foreach sanitizer
        if(NOT "${SANTIZERS}" STREQUAL "")
        foreach(SANTIZER ${SANTIZERS})

            message("Set target: catch_main_${SANTIZER}")

            # Add libc++ flag if memory sanitizer
            set(COMPILER_FLAGS_ "${COMPILER_FLAGS}")
            if ("${SANTIZER}" STREQUAL "memory" AND INSTRUMENTED_FOUND)
                set(COMPILER_FLAGS_ ${COMPILER_FLAGS_} ${INSTRUMENTED_COMPILER_FLAGS})
            endif()

            # Build obj
            add_library(catch_main_obj_${SANTIZER} OBJECT ${CATCH_MAIN_SRC})
            target_include_directories(catch_main_obj_${SANTIZER} PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
            target_compile_options(catch_main_obj_${SANTIZER} PRIVATE ${COMPILER_FLAGS_} -fsanitize=${SANTIZER})
            set_target_properties(catch_main_obj_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET catch_main_obj_${SANTIZER} PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            set_definitions(catch_main_obj_${SANTIZER} ${SANTIZER})

        endforeach()
        endif()

        # Build raw binary with BUILD_VALGRIND
        if(BUILD_WITH_VALGRIND OR SUPPORT_COVERAGE)

            message("Set target: catch_main_valgrind")

            # Build obj
            add_library(catch_main_obj_valgrind OBJECT ${CATCH_MAIN_SRC})
            target_include_directories(catch_main_obj_valgrind PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
            target_compile_options(catch_main_obj_valgrind PRIVATE ${COMPILER_FLAGS})
            set_target_properties(catch_main_obj_valgrind PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET catch_main_obj_valgrind PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            target_compile_definitions(catch_main_obj_valgrind PRIVATE PRIVATE BUILD_VALGRIND)
            set_definitions(catch_main_obj_valgrind "")

        endif()

    endif()

endif()