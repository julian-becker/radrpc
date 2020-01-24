set(COMPILE_WARNINGS 
    "-Wuninitialized"
    "-Wconversion"
    "-pedantic-errors"
    "-Wold-style-cast"
    "-Wunreachable-code"
    "-Wall"
    "-Wno-unused"
    "-Wno-unknown-pragmas"
    "-Wno-keyword-macro")

# Need to set global flags, target_compile_options doesn't seem to work with coverage
if (SUPPORT_COVERAGE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        clang_append_coverage_compiler_flags()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        gnu_append_coverage_compiler_flags()
    endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set(COMPILER_FLAGS -pthread ${COMPILE_WARNINGS})
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        message("Build type: RelWithDebInfo")
        set(COMPILER_FLAGS ${COMPILER_FLAGS} -g)
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        message("Build type: Debug")
        set(COMPILER_FLAGS ${COMPILER_FLAGS} -O0 -g -fno-omit-frame-pointer -fno-sanitize-recover=all)
    endif()

    set(INSTRUMENTED_COMPILER_FLAGS -stdlib=libc++ -fsanitize-blacklist=${SANITIZER_SUPPRESSION})
    set(LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
    set(LINK_LIBRARIES
            ${Boost_LIBRARIES})
    set(INSTRUMENTED_LINK_LIBRARIES
            ${LIBCPP_LIBRARIES}
            ${BOOSTC_LIBRARIES})
    if(SUPPORT_SSL)
        set(LINK_LIBRARIES ${OPENSSL_LIBRARIES} ${LINK_LIBRARIES})
        set(INSTRUMENTED_LINK_LIBRARIES 
            ${LIBCPP_LIBRARIES}
            ${OPENSSLC_LIBRARIES}
            ${BOOSTC_LIBRARIES})
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set(COMPILER_FLAGS -pthread ${COMPILE_WARNINGS})
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        message("Build type: RelWithDebInfo")
        set(COMPILER_FLAGS ${COMPILER_FLAGS} -g)
    elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        message("Build type: Debug")
        set(COMPILER_FLAGS ${COMPILER_FLAGS} -O0 -g -fno-omit-frame-pointer -fno-sanitize-recover=all)
    endif()
    set(LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
    set(LINK_LIBRARIES
            ${Boost_LIBRARIES})
    if(SUPPORT_SSL)
        set(LINK_LIBRARIES ${OPENSSL_LIBRARIES} ${LINK_LIBRARIES})
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") # todo set flags by type
    
    set(COMPILER_FLAGS /MP4 /MD /Od /bigobj)
    STRING(REPLACE "/O0" "/Od" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    set(LINK_LIBRARIES
            ${Boost_LIBRARIES})
    if(SUPPORT_SSL)
        set(LINK_LIBRARIES ${OPENSSL_LIBRARIES} ${LINK_LIBRARIES})
    endif()

endif()