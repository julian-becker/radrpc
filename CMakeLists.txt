cmake_minimum_required (VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json" DESTINATION "${PROJECT_SOURCE_DIR}")
endif()
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(Scripts)
set(CMAKE_CXX_STANDARD 14)
set(REPORT_FOLDER "Reports")




option(BUILD_EXAMPLES
        "Build all examples located in folder 'examples'"
        ON)
option(BUILD_TESTS
        "Build common tests to check the implementation"
        ON)
option(BUILD_STRESS_TESTS
        "Build all stress tests"
        ON)
option(BIN_ASAN
        "Build targets with AddressSanitizer"
        ON)
option(BIN_UBSAN
        "Build targets with UndefinedBehaviorSanitizer"
        ON)
option(BIN_TSAN
        "Build targets with ThreadSanitizer"
        ON)
option(BIN_VALGRIND
        "Build targets configured for Valgrind"
        ON)
option(BIN_RAW
        "Build targets without sanitizer instrumentation"
        ON)
option(LOGGING
        "Enables logging for debug purposes"
        ON)
option(SUPPORT_COVERAGE
        "Build targets with code coverage, this setting only affects compilers with clang or gnu"
        ON)
option(SUPPORT_SSL
        "Configures for ssl usage with OpenSSL"
        ON)
option(CREATE_SCRIPTS
        "Create scripts to run the tests, this setting doesn't affect msvc compiler"
        ON)
message("${BUILD_EXAMPLES}\tBuild examples")
message("${BUILD_TESTS}\tBuild tests")
message("${BUILD_STRESS_TESTS}\tBuild stress tests")
message("${BIN_ASAN}\tBinaries AddressSanitizer")
message("${BIN_UBSAN}\tBinaries UndefinedBehaviorSanitizer")
message("${BIN_TSAN}\tBinaries ThreadSanitizer")
message("${BIN_VALGRIND}\tBinaries Valgrind")
message("${BIN_RAW}\tBinaries non-instrumented")
message("${LOGGING}\tLogging")
message("${SUPPORT_COVERAGE}\tCoverage support")
message("${SUPPORT_SSL}\tSSL support")
message("${CREATE_SCRIPTS}\tCreate test scripts")




find_package(Boost 1.70 REQUIRED COMPONENTS filesystem system date_time)
if (NOT Boost_FOUND)
    return()
endif()

if(SUPPORT_SSL)
    find_package(OpenSSL REQUIRED)
    if(NOT OpenSSL_FOUND)
        message(WARNING "It was requested to build with SSL support but OpenSSL was not found!")
        set(SUPPORT_SSL OFF)
    endif()
endif()




# Need to set global flags, target_compile_options doesn't seem to work with coverage
if (SUPPORT_COVERAGE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    endif()
endif()
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    set(GLOBAL_COMPILER_FLAGS -O0 -g -pthread -fno-omit-frame-pointer -fno-sanitize-recover=all)
    set(GLOBAL_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
    set(GLOBAL_LINK_LIBRARIES
            ${Boost_LIBRARIES})

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    set(GLOBAL_COMPILER_FLAGS -O0 -g -pthread -fno-omit-frame-pointer -fno-sanitize-recover=all)
    set(GLOBAL_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -rdynamic")
    set(GLOBAL_LINK_LIBRARIES
            ${Boost_LIBRARIES})

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    
    set(GLOBAL_COMPILER_FLAGS /MP4 /MD /Od /bigobj)
    STRING(REPLACE "/O0" "/Od" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    set(GLOBAL_LINK_LIBRARIES
        ${Boost_LIBRARIES})

endif()




if (SUPPORT_SSL)
    include_directories(
            "${PROJECT_SOURCE_DIR}"
            "${PROJECT_SOURCE_DIR}/include/"
            "${PROJECT_SOURCE_DIR}/test/"
            "${PROJECT_SOURCE_DIR}/test/dep/"
            "${OPENSSL_INCLUDE_DIR}"
            "${Boost_INCLUDE_DIRS}")
else()
    include_directories(
            ${PROJECT_SOURCE_DIR}
            "${PROJECT_SOURCE_DIR}/include/"
            "${PROJECT_SOURCE_DIR}/test/"
            "${PROJECT_SOURCE_DIR}/test/dep/"
            "${Boost_INCLUDE_DIRS}")
endif()




function(set_definitions TARGET SANTIZER_STRING)
    if(SUPPORT_SSL)
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} RADRPC_SSL_SUPPORT)
    endif()
    if(LOGGING)
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} RADRPC_LOGGING)
    endif()
    if(NOT "${SANTIZER_STRING}" STREQUAL "")
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} SANITIZER_ACTIVE)
    endif()
    # Workaround for UndefinedBehaviorSanitizer, got no __has_feature (just used for tests)
    if("${SANTIZER_STRING}" STREQUAL "address")
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} ASAN)
    endif()
    if("${SANTIZER_STRING}" STREQUAL "undefined")
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} UBSAN)
    endif()
    if("${SANTIZER_STRING}" STREQUAL "memory")
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} MSAN)
    endif()
    if("${SANTIZER_STRING}" STREQUAL "thread")
        set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} TSAN)
    endif()
    target_compile_definitions(${TARGET} PRIVATE ${COMPILE_DEFINITIONS})
endfunction()




set(SANTIZERS "")
if(BIN_ASAN)
    set(SANTIZERS ${SANTIZERS} address)
endif()
if(BIN_UBSAN)
    set(SANTIZERS ${SANTIZERS} undefined)
endif()
if(BIN_TSAN)
    set(SANTIZERS ${SANTIZERS} thread)
endif()




if(NOT "${SANTIZERS}" STREQUAL "" AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    foreach(SANTIZER ${SANTIZERS})

        if (BUILD_TESTS)
            # unit_common
            message("Set target: unit_common_${SANTIZER}")
            if (SUPPORT_SSL)
                add_executable(unit_common_${SANTIZER} 
                    "${PROJECT_SOURCE_DIR}/test/_main.cpp"
                    "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_client.cpp"
                    "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_server.cpp"
                    "${PROJECT_SOURCE_DIR}/test/defaults/default_ssl_client.cpp"
                    "${PROJECT_SOURCE_DIR}/test/defaults/default_ssl_server.cpp"
                    "${PROJECT_SOURCE_DIR}/test/misc.cpp"
                    "${PROJECT_SOURCE_DIR}/test/plain_client.cpp"
                    "${PROJECT_SOURCE_DIR}/test/plain_server.cpp"
                    "${PROJECT_SOURCE_DIR}/test/ssl_client.cpp"
                    "${PROJECT_SOURCE_DIR}/test/ssl_server.cpp")
            else()
                add_executable(unit_common_${SANTIZER} 
                    "${PROJECT_SOURCE_DIR}/test/_main.cpp"
                    "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_client.cpp"
                    "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_server.cpp"
                    "${PROJECT_SOURCE_DIR}/test/misc.cpp"
                    "${PROJECT_SOURCE_DIR}/test/client_plain.cpp"
                    "${PROJECT_SOURCE_DIR}/test/server_plain.cpp")
            endif()
            target_compile_options(unit_common_${SANTIZER} PRIVATE ${GLOBAL_COMPILER_FLAGS} -fsanitize=${SANTIZER})
            set_target_properties(unit_common_${SANTIZER} PROPERTIES LINK_FLAGS "-fsanitize=${SANTIZER} ${GLOBAL_LINKER_FLAGS}")
            set_definitions(unit_common_${SANTIZER} ${SANTIZER})
            target_link_libraries(unit_common_${SANTIZER} ${GLOBAL_LINK_LIBRARIES})
            if (SUPPORT_SSL)
                target_link_libraries(unit_common_${SANTIZER} ${OPENSSL_LIBRARIES})
            endif()
            # unit_utils
            message("Set target: unit_utils_${SANTIZER}")
            add_executable(unit_utils_${SANTIZER} "${PROJECT_SOURCE_DIR}/test/unit_utils.cpp")
            target_compile_options(unit_utils_${SANTIZER} PRIVATE ${GLOBAL_COMPILER_FLAGS} -fsanitize=${SANTIZER})
            set_target_properties(unit_utils_${SANTIZER} PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS} -fsanitize=${SANTIZER}")
            set_definitions(unit_utils_${SANTIZER} ${SANTIZER})
            target_link_libraries(unit_utils_${SANTIZER} ${GLOBAL_LINK_LIBRARIES})
            if (SUPPORT_SSL)
                target_link_libraries(unit_utils_${SANTIZER} ${OPENSSL_LIBRARIES})
            endif()
        endif()

        if (BUILD_STRESS_TESTS)

            # stress_client
            message("Set target: stress_client_${SANTIZER}")
            add_executable(stress_client_${SANTIZER} "${PROJECT_SOURCE_DIR}/test/stress_client.cpp")
            target_compile_options(stress_client_${SANTIZER} PRIVATE ${GLOBAL_COMPILER_FLAGS} -fsanitize=${SANTIZER})
            set_target_properties(stress_client_${SANTIZER} PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS} -fsanitize=${SANTIZER}")
            set_definitions(stress_client_${SANTIZER} ${SANTIZER})
            target_link_libraries(stress_client_${SANTIZER} ${GLOBAL_LINK_LIBRARIES})
            if (SUPPORT_SSL)
                target_link_libraries(stress_client_${SANTIZER} ${OPENSSL_LIBRARIES})
            endif()
            # stress_server
            message("Set target: stress_server_${SANTIZER}")
            add_executable(stress_server_${SANTIZER} "${PROJECT_SOURCE_DIR}/test/stress_server.cpp")
            target_compile_options(stress_server_${SANTIZER} PRIVATE ${GLOBAL_COMPILER_FLAGS} -fsanitize=${SANTIZER})
            set_target_properties(stress_server_${SANTIZER} PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS} -fsanitize=${SANTIZER}")
            set_definitions(stress_server_${SANTIZER} ${SANTIZER})
            target_link_libraries(stress_server_${SANTIZER} ${GLOBAL_LINK_LIBRARIES})
            if (SUPPORT_SSL)
                target_link_libraries(stress_server_${SANTIZER} ${OPENSSL_LIBRARIES})
            endif()

        endif()

    endforeach()
endif()

if (BIN_RAW OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    if (BUILD_TESTS)
        # unit_common
        message("Set target: unit_common")
        if (SUPPORT_SSL)
            add_executable(unit_common 
                "${PROJECT_SOURCE_DIR}/test/_main.cpp"
                "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_client.cpp"
                "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_server.cpp"
                "${PROJECT_SOURCE_DIR}/test/defaults/default_ssl_client.cpp"
                "${PROJECT_SOURCE_DIR}/test/defaults/default_ssl_server.cpp"
                "${PROJECT_SOURCE_DIR}/test/misc.cpp"
                "${PROJECT_SOURCE_DIR}/test/plain_client.cpp"
                "${PROJECT_SOURCE_DIR}/test/plain_server.cpp"
                "${PROJECT_SOURCE_DIR}/test/ssl_client.cpp"
                "${PROJECT_SOURCE_DIR}/test/ssl_server.cpp")
        else()
            add_executable(unit_common 
                "${PROJECT_SOURCE_DIR}/test/_main.cpp"
                "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_client.cpp"
                "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_server.cpp"
                "${PROJECT_SOURCE_DIR}/test/misc.cpp"
                "${PROJECT_SOURCE_DIR}/test/client_plain.cpp"
                "${PROJECT_SOURCE_DIR}/test/server_plain.cpp")
        endif()
        target_compile_options(unit_common PRIVATE ${GLOBAL_COMPILER_FLAGS})
        set_target_properties(unit_common PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
        set_definitions(unit_common "")
        target_link_libraries(unit_common ${GLOBAL_LINK_LIBRARIES})
        if (SUPPORT_SSL)
            target_link_libraries(unit_common ${OPENSSL_LIBRARIES})
        endif()
        # unit_utils
        message("Set target: unit_utils")
        add_executable(unit_utils "${PROJECT_SOURCE_DIR}/test/unit_utils.cpp")
        target_compile_options(unit_utils PRIVATE ${GLOBAL_COMPILER_FLAGS})
        set_target_properties(unit_utils PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
        set_definitions(unit_utils "")
        target_link_libraries(unit_utils ${GLOBAL_LINK_LIBRARIES})
        if (SUPPORT_SSL)
            target_link_libraries(unit_utils ${OPENSSL_LIBRARIES})
        endif()
    endif()

    if (BUILD_STRESS_TESTS)
        # stress_client
        message("Set target: stress_client")
        add_executable(stress_client "${PROJECT_SOURCE_DIR}/test/stress_client.cpp")
        target_compile_options(stress_client PRIVATE ${GLOBAL_COMPILER_FLAGS})
        set_target_properties(stress_client PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
        set_definitions(stress_client "")
        target_link_libraries(stress_client ${GLOBAL_LINK_LIBRARIES})
        if (SUPPORT_SSL)
            target_link_libraries(stress_client ${OPENSSL_LIBRARIES})
        endif()
        # stress_server
        message("Set target: stress_server")
        add_executable(stress_server "${PROJECT_SOURCE_DIR}/test/stress_server.cpp")
        target_compile_options(stress_server PRIVATE ${GLOBAL_COMPILER_FLAGS})
        set_target_properties(stress_server PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
        set_definitions(stress_server "")
        target_link_libraries(stress_server ${GLOBAL_LINK_LIBRARIES})
        if (SUPPORT_SSL)
            target_link_libraries(stress_server ${OPENSSL_LIBRARIES})
        endif()
    endif()

endif()

if (BUILD_TESTS AND BIN_VALGRIND AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # unit_common_valgrind
    message("Set target: unit_common_valgrind")
    if (SUPPORT_SSL)
        add_executable(unit_common_valgrind 
            "${PROJECT_SOURCE_DIR}/test/_main.cpp"
            "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_client.cpp"
            "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_server.cpp"
            "${PROJECT_SOURCE_DIR}/test/defaults/default_ssl_client.cpp"
            "${PROJECT_SOURCE_DIR}/test/defaults/default_ssl_server.cpp"
            "${PROJECT_SOURCE_DIR}/test/misc.cpp"
            "${PROJECT_SOURCE_DIR}/test/plain_client.cpp"
            "${PROJECT_SOURCE_DIR}/test/plain_server.cpp"
            "${PROJECT_SOURCE_DIR}/test/ssl_client.cpp"
            "${PROJECT_SOURCE_DIR}/test/ssl_server.cpp")
    else()
        add_executable(unit_common_valgrind 
            "${PROJECT_SOURCE_DIR}/test/_main.cpp"
            "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_client.cpp"
            "${PROJECT_SOURCE_DIR}/test/defaults/default_plain_server.cpp"
            "${PROJECT_SOURCE_DIR}/test/misc.cpp"
            "${PROJECT_SOURCE_DIR}/test/client_plain.cpp"
            "${PROJECT_SOURCE_DIR}/test/server_plain.cpp")
    endif()
    target_compile_options(unit_common_valgrind PRIVATE ${GLOBAL_COMPILER_FLAGS})
    set_target_properties(unit_common_valgrind PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
    set_definitions(unit_common_valgrind "")
    target_compile_definitions(unit_common_valgrind PRIVATE PRIVATE BUILD_VALGRIND)
    target_link_libraries(unit_common_valgrind ${GLOBAL_LINK_LIBRARIES})
    if (SUPPORT_SSL)
        target_link_libraries(unit_common_valgrind ${OPENSSL_LIBRARIES})
    endif()
    # unit_utils_valgrind
    message("Set target: unit_utils_valgrind")
    add_executable(unit_utils_valgrind "${PROJECT_SOURCE_DIR}/test/unit_utils.cpp")
    target_compile_options(unit_utils_valgrind PRIVATE ${GLOBAL_COMPILER_FLAGS})
    set_target_properties(unit_utils_valgrind PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
    set_definitions(unit_utils_valgrind "")
    target_compile_definitions(unit_utils_valgrind PRIVATE PRIVATE BUILD_VALGRIND)
    target_link_libraries(unit_utils_valgrind ${GLOBAL_LINK_LIBRARIES})
    if (SUPPORT_SSL)
        target_link_libraries(unit_utils_valgrind ${OPENSSL_LIBRARIES})
    endif()
endif()

if (BUILD_EXAMPLES)
    # client
    message("Set target: client")
    add_executable(client "${PROJECT_SOURCE_DIR}/examples/basic/client.cpp")
    target_compile_options(client PRIVATE ${GLOBAL_COMPILER_FLAGS})
    set_target_properties(client PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
    set_definitions(client "")
    target_link_libraries(client ${GLOBAL_LINK_LIBRARIES})
    if (SUPPORT_SSL)
        target_link_libraries(client ${OPENSSL_LIBRARIES})
    endif()
    # server
    message("Set target: server")
    add_executable(server "${PROJECT_SOURCE_DIR}/examples/basic/server.cpp")
    target_compile_options(server PRIVATE ${GLOBAL_COMPILER_FLAGS})
    set_target_properties(server PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
    set_definitions(server "")
    target_link_libraries(server ${GLOBAL_LINK_LIBRARIES})
    if (SUPPORT_SSL)
        target_link_libraries(server ${OPENSSL_LIBRARIES})
    endif()
    # advanced_client
    message("Set target: advanced_client")
    add_executable(advanced_client "${PROJECT_SOURCE_DIR}/examples/advanced/advanced_client.cpp")
    target_compile_options(advanced_client PRIVATE ${GLOBAL_COMPILER_FLAGS})
    set_target_properties(advanced_client PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
    set_definitions(advanced_client "")
    target_link_libraries(advanced_client ${GLOBAL_LINK_LIBRARIES})
    if (SUPPORT_SSL)
        target_link_libraries(advanced_client ${OPENSSL_LIBRARIES})
    endif()
    # advanced_server
    message("Set target: advanced_server")
    add_executable(advanced_server "${PROJECT_SOURCE_DIR}/examples/advanced/advanced_server.cpp")
    target_compile_options(advanced_server PRIVATE ${GLOBAL_COMPILER_FLAGS})
    set_target_properties(advanced_server PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
    set_definitions(advanced_server "")
    target_link_libraries(advanced_server ${GLOBAL_LINK_LIBRARIES})
    if (SUPPORT_SSL)
        target_link_libraries(advanced_server ${OPENSSL_LIBRARIES})
    endif()
    
    if (SUPPORT_SSL)
    
        # ssl_client
        message("Set target: ssl_client")
        add_executable(ssl_client "${PROJECT_SOURCE_DIR}/examples/ssl/ssl_client.cpp")
        target_compile_options(ssl_client PRIVATE ${GLOBAL_COMPILER_FLAGS})
        set_target_properties(ssl_client PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
        set_definitions(ssl_client "")
        target_link_libraries(ssl_client ${GLOBAL_LINK_LIBRARIES})
        if (SUPPORT_SSL)
            target_link_libraries(ssl_client ${OPENSSL_LIBRARIES})
        endif()
        # ssl_server
        message("Set target: ssl_server")
        add_executable(ssl_server "${PROJECT_SOURCE_DIR}/examples/ssl/ssl_server.cpp")
        target_compile_options(ssl_server PRIVATE ${GLOBAL_COMPILER_FLAGS})
        set_target_properties(ssl_server PROPERTIES LINK_FLAGS "${GLOBAL_LINKER_FLAGS}")
        set_definitions(ssl_server "")
        target_link_libraries(ssl_server ${GLOBAL_LINK_LIBRARIES})
        if (SUPPORT_SSL)
            target_link_libraries(ssl_server ${OPENSSL_LIBRARIES})
        endif()

    endif()

endif()




if (CREATE_SCRIPTS AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # unit_common
    create_script_asan(unit_common_address "${REPORT_FOLDER}")
    create_script_tsan(unit_common_thread "${REPORT_FOLDER}")
    create_script_ubsan(unit_common_undefined "${REPORT_FOLDER}")
    create_script_valgrind(unit_common_valgrind "${REPORT_FOLDER}")
    create_script_coverage(unit_common "${PROJECT_SOURCE_DIR}/include")
    package_scripts("s_unit_common_all" ON 
        "s_unit_common_address.sh"
        "s_unit_common_thread.sh"
        "s_unit_common_undefined.sh"
        "s_unit_common_valgrind.sh"
        "s_unit_common_cover.sh")
    # unit_utils 
    create_script_asan(unit_utils_address "${REPORT_FOLDER}")
    create_script_tsan(unit_utils_thread "${REPORT_FOLDER}")
    create_script_ubsan(unit_utils_undefined "${REPORT_FOLDER}")
    create_script_valgrind(unit_utils_valgrind "${REPORT_FOLDER}")
    package_scripts("s_unit_utils_all" ON 
        "s_unit_utils_address.sh"
        "s_unit_utils_thread.sh"
        "s_unit_utils_undefined.sh"
        "s_unit_utils_valgrind.sh")
    # stress_client
    create_script_asan(stress_client_address "${REPORT_FOLDER}")
    create_script_tsan(stress_client_thread "${REPORT_FOLDER}")
    create_script_ubsan(stress_client_undefined "${REPORT_FOLDER}")
    package_scripts("s_stress_client_all" ON 
        "s_stress_client_address.sh"
        "s_stress_client_thread.sh"
        "s_stress_client_undefined.sh")
    # stress_server
    create_script_asan(stress_server_address "${REPORT_FOLDER}")
    create_script_tsan(stress_server_thread "${REPORT_FOLDER}")
    create_script_ubsan(stress_server_undefined "${REPORT_FOLDER}")
    package_scripts("s_stress_server_all" ON 
        "s_stress_server_address.sh"
        "s_stress_server_thread.sh"
        "s_stress_server_undefined.sh")
endif()
