set(COMPILE_WARNINGS "-Wuninitialized")

# Need to set global flags, target_compile_options doesn't seem to work with coverage
if (SUPPORT_COVERAGE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # todo currently doesn't work with shared library
        # set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set(COMPILER_FLAGS -O0 -g -pthread -fno-omit-frame-pointer -fno-sanitize-recover=all ${COMPILE_WARNINGS})
    set(INSTRUMENTED_COMPILER_FLAGS -stdlib=libc++ -fsanitize-blacklist=${SANITIZER_SUPPRESSION})
    set(LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
    set(LINK_LIBRARIES
            ${Boost_LIBRARIES})
    set(INSTRUMENTED_LINK_LIBRARIES
            ${LIBCPP_LIBRARIES}
            ${BOOSTC_LIBRARIES})
    if(SUPPORT_SSL)
        set(LINK_LIBRARIES ${LINK_LIBRARIES} ${OPENSSL_LIBRARIES})
        set(INSTRUMENTED_LINK_LIBRARIES 
            ${LIBCPP_LIBRARIES}
            ${OPENSSLC_LIBRARIES}
            ${BOOSTC_LIBRARIES})
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set(COMPILER_FLAGS -O0 -g -pthread -fno-omit-frame-pointer -fno-sanitize-recover=all ${COMPILE_WARNINGS})
    set(LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
    set(LINK_LIBRARIES
            ${Boost_LIBRARIES})
    if(SUPPORT_SSL)
        set(LINK_LIBRARIES ${LINK_LIBRARIES} ${OPENSSL_LIBRARIES})
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    
    set(COMPILER_FLAGS /MP4 /MD /Od /bigobj)
    STRING(REPLACE "/O0" "/Od" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    set(LINK_LIBRARIES
        ${Boost_LIBRARIES})

endif()