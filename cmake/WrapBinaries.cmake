if (CREATE_SCRIPTS AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    # unit_client
    create_script_asan(unit_client_address "${REPORT_DIR}")
    create_script_tsan(unit_client_thread "${REPORT_DIR}")
    create_script_ubsan(unit_client_undefined "${REPORT_DIR}")
    create_script_msan(unit_client_memory "${REPORT_DIR}")
    create_script_valgrind(unit_client_valgrind "${REPORT_DIR}")

    # unit_server
    create_script_asan(unit_server_address "${REPORT_DIR}")
    create_script_tsan(unit_server_thread "${REPORT_DIR}")
    create_script_ubsan(unit_server_undefined "${REPORT_DIR}")
    create_script_msan(unit_server_memory "${REPORT_DIR}")
    create_script_valgrind(unit_server_valgrind "${REPORT_DIR}")

    # unit_misc
    create_script_asan(unit_misc_address "${REPORT_DIR}")
    create_script_tsan(unit_misc_thread "${REPORT_DIR}")
    create_script_ubsan(unit_misc_undefined "${REPORT_DIR}")
    create_script_msan(unit_misc_memory "${REPORT_DIR}")
    create_script_valgrind(unit_misc_valgrind "${REPORT_DIR}")

    # unit_self
    create_script_asan(unit_self_address "${REPORT_DIR}")
    create_script_tsan(unit_self_thread "${REPORT_DIR}")
    create_script_ubsan(unit_self_undefined "${REPORT_DIR}")
    create_script_msan(unit_self_memory "${REPORT_DIR}")
    create_script_valgrind(unit_self_valgrind "${REPORT_DIR}")

    # unit_radrpc_address
    package_scripts("s_unit_radrpc_address" ON 
        "s_unit_client_address.sh"
        "s_unit_server_address.sh"
        "s_unit_misc_address.sh"
        "s_unit_self_address.sh")

    # unit_radrpc_thread
    package_scripts("s_unit_radrpc_thread" ON 
        "s_unit_client_thread.sh"
        "s_unit_server_thread.sh"
        "s_unit_misc_thread.sh"
        "s_unit_self_thread.sh")

    # unit_radrpc_undefined
    package_scripts("s_unit_radrpc_undefined" ON 
        "s_unit_client_undefined.sh"
        "s_unit_server_undefined.sh"
        "s_unit_misc_undefined.sh"
        "s_unit_self_undefined.sh")

    # unit_radrpc_memory
    package_scripts("s_unit_radrpc_memory" ON 
        "s_unit_client_memory.sh"
        "s_unit_server_memory.sh"
        "s_unit_misc_memory.sh"
        "s_unit_self_memory.sh")

    # unit_radrpc_valgrind
    package_scripts("s_unit_radrpc_valgrind" ON 
        "s_unit_client_valgrind.sh"
        "s_unit_server_valgrind.sh"
        "s_unit_misc_valgrind.sh"
        "s_unit_self_valgrind.sh")

    # stress_client
    create_script_asan(stress_client_address "${REPORT_DIR}")
    create_script_tsan(stress_client_thread "${REPORT_DIR}")
    create_script_ubsan(stress_client_undefined "${REPORT_DIR}")
    create_script_ubsan(stress_client_memory "${REPORT_DIR}")
    package_scripts("s_stress_client_all" ON 
        "s_stress_client_address.sh"
        "s_stress_client_thread.sh"
        "s_stress_client_undefined.sh"
        "s_stress_client_memory.sh")
        
    # stress_server
    create_script_asan(stress_server_address "${REPORT_DIR}")
    create_script_tsan(stress_server_thread "${REPORT_DIR}")
    create_script_ubsan(stress_server_undefined "${REPORT_DIR}")
    create_script_msan(stress_server_memory "${REPORT_DIR}")
    package_scripts("s_stress_server_all" ON 
        "s_stress_server_address.sh"
        "s_stress_server_thread.sh"
        "s_stress_server_undefined.sh"
        "s_stress_server_memory.sh")

endif()
