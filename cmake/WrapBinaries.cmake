if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

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