if (BUILD_TESTS OR BUILD_STRESS_TESTS)

    set(CATCH_MAIN_SRC      
            "${TEST_DIR}/unit/catch_main.cpp")

    # Build raw on windows
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

        message("Set target: catch_main")

        # Build obj
        add_library(catch_main_obj OBJECT ${CATCH_MAIN_SRC})
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

            # Build obj
            add_library(catch_main_obj_${SANTIZER} OBJECT ${CATCH_MAIN_SRC})
            target_compile_options(catch_main_obj_${SANTIZER} PRIVATE ${COMPILER_FLAGS} -fsanitize=${SANTIZER})
            set_target_properties(catch_main_obj_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET catch_main_obj_${SANTIZER} PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            set_definitions(catch_main_obj_${SANTIZER} ${SANTIZER})

        endforeach()
        endif()

        if (BIN_MSAN AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

        endif()

        # Build raw binary with BUILD_VALGRIND
        if(BIN_VALGRIND)

            message("Set target: catch_main_valgrind")

            # Build obj
            add_library(catch_main_valgrind_obj OBJECT ${CATCH_MAIN_SRC})
            target_compile_options(catch_main_valgrind_obj PRIVATE ${COMPILER_FLAGS})
            set_target_properties(catch_main_valgrind_obj PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET catch_main_valgrind_obj PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            target_compile_definitions(catch_main_valgrind_obj PRIVATE PRIVATE BUILD_VALGRIND)
            set_definitions(catch_main_valgrind_obj "")

        endif()

    endif()

endif()