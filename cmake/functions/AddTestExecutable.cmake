function(add_test_executable BINARY SRC_FILES IMPL_CATCH)

    # Build raw on windows
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

        message("Set target: ${BINARY}")

        if (IMPL_CATCH)
            add_executable(${BINARY} ${SRC_FILES} $<TARGET_OBJECTS:catch_main_obj>)
        else()
            add_executable(${BINARY} ${SRC_FILES})
        endif()
        target_include_directories(${BINARY} PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
        target_compile_options(${BINARY} PRIVATE ${COMPILER_FLAGS})
        set_target_properties(${BINARY} PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
        if(BUILD_INTERNAL_SHARED)
            set_property(TARGET ${BINARY} PROPERTY POSITION_INDEPENDENT_CODE 1)
        endif()
        set_definitions(${BINARY} "")
        if(BUILD_INTERNAL_SHARED)
            # Link shared library
            target_link_libraries(${BINARY} ${LINK_LIBRARIES} radrpc_shared test_shared)
        else()
            # Link static library
            target_link_libraries(${BINARY} ${LINK_LIBRARIES} radrpc_static test_static)
        endif()

    # Build instrumted on other platforms
    else()

        # Build instrumted executable foreach sanitizer
        if(NOT "${SANTIZERS}" STREQUAL "")
        foreach(SANTIZER ${SANTIZERS})

            message("Set target: ${BINARY}_${SANTIZER}")

            # Add libc++ flag & link instrumented if memory sanitizer is used
            set(COMPILER_FLAGS_ "${COMPILER_FLAGS}")
            set(LINK_LIBRARIES_ "${LINK_LIBRARIES}")
            if ("${SANTIZER}" STREQUAL "memory" AND INSTRUMENTED_FOUND)
                set(COMPILER_FLAGS_ ${COMPILER_FLAGS_} ${INSTRUMENTED_COMPILER_FLAGS})
                set(LINK_LIBRARIES_ ${INSTRUMENTED_LINK_LIBRARIES})
            endif()

            if (IMPL_CATCH)
                add_executable(${BINARY}_${SANTIZER} ${SRC_FILES} $<TARGET_OBJECTS:catch_main_obj_${SANTIZER}>)
            else()
                add_executable(${BINARY}_${SANTIZER} ${SRC_FILES})
            endif()
            target_include_directories(${BINARY}_${SANTIZER} PRIVATE ${RADRPC_INCLUDE} ${RADRPC_TEST_INCLUDE})
            target_compile_options(${BINARY}_${SANTIZER} PRIVATE ${COMPILER_FLAGS_} -fsanitize=${SANTIZER})
            set_target_properties(${BINARY}_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${LINKER_FLAGS}")
            if (BUILD_INTERNAL_SHARED)
                set_property(TARGET ${BINARY}_${SANTIZER} PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            set_definitions(${BINARY}_${SANTIZER} ${SANTIZER})
            if(BUILD_INTERNAL_SHARED)
                # Link shared library
                target_link_libraries(${BINARY}_${SANTIZER} ${LINK_LIBRARIES_} radrpc_shared_${SANTIZER} test_shared_${SANTIZER})
            else()
                # Link static library
                target_link_libraries(${BINARY}_${SANTIZER} ${LINK_LIBRARIES_} radrpc_static_${SANTIZER} test_static_${SANTIZER})
            endif()

        endforeach()
        endif()

        # Build raw binary with BUILD_VALGRIND
        if(BIN_VALGRIND)

            message("Set target: ${BINARY}_valgrind")

            if (IMPL_CATCH)
                add_executable(${BINARY}_valgrind ${SRC_FILES} $<TARGET_OBJECTS:catch_main_obj_valgrind>)
            else()
                add_executable(${BINARY}_valgrind ${SRC_FILES})
            endif()
            target_compile_options(${BINARY}_valgrind PRIVATE ${COMPILER_FLAGS})
            set_target_properties(${BINARY}_valgrind PROPERTIES LINK_FLAGS "${LINKER_FLAGS}")
            if(BUILD_INTERNAL_SHARED)
                set_property(TARGET ${BINARY}_valgrind PROPERTY POSITION_INDEPENDENT_CODE 1)
            endif()
            target_compile_definitions(${BINARY}_valgrind PRIVATE PRIVATE BUILD_VALGRIND)
            set_definitions(${BINARY}_valgrind "")
            if(BUILD_INTERNAL_SHARED)
                # Link shared library
                target_link_libraries(${BINARY}_valgrind ${LINK_LIBRARIES} radrpc_shared test_valgrind_shared)
            else()
                # Link static library
                target_link_libraries(${BINARY}_valgrind ${LINK_LIBRARIES} radrpc_static test_valgrind_static)
            endif()

        endif()

    endif()

endfunction()