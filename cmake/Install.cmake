if(UNIX)

    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(CONFIG_PACKAGE_LOCATION lib/cmake/radrpc)
    set(INCLUDE_INSTALL_DIR include/)

    # Install static / shared library
    install(
        TARGETS 
            radrpc_static radrpc_shared 
        DESTINATION 
            lib 
        EXPORT 
            radrpcTargets)
    install(
        EXPORT 
            radrpcTargets
        DESTINATION 
            ${CMAKE_INSTALL_LIBDIR}/cmake/radrpc
        NAMESPACE 
            radrpc::)

    # Install headers
    install(
        DIRECTORY include/ # slash = copy contents
        DESTINATION include
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.in" EXCLUDE)

    # Write config version
    configure_file(
        "${PROJECT_SOURCE_DIR}/include/radrpc/version.hpp.in"
        "${PROJECT_SOURCE_DIR}/include/radrpc/version.hpp")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/radrpcConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)

    # Write target config
    # radrpcConfig.cmake        <--- generated
    #           |
    #           v
    # radrpcConfig.cmake.in     <--- user can use variables from radrpcTargets.cmake
    #                                to define for e.g.
    #                                'RADRPC_LIBRARIES' 'RADRPC_INCLUDE_DIR'
    configure_package_config_file(
        cmake/radrpcConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/radrpcConfig.cmake
        INSTALL_DESTINATION ${CONFIG_PACKAGE_LOCATION}
        PATH_VARS INCLUDE_INSTALL_DIR)

    # Generate targets providing the built libraries
    export(
        EXPORT 
            radrpcTargets
        FILE 
            "${CMAKE_CURRENT_BINARY_DIR}/cmake/radrpcTargets.cmake"
        NAMESPACE 
            radrpc::)

    install(
        EXPORT 
            radrpcTargets
        FILE 
            radrpcTargets.cmake
        NAMESPACE 
            radrpc::
        DESTINATION
            ${CONFIG_PACKAGE_LOCATION})

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/radrpcConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/cmake/radrpcConfigVersion.cmake
        DESTINATION 
            ${CONFIG_PACKAGE_LOCATION})

elseif(WIN32)



else()
    message(FATAL_ERROR "Could not set install for this platform")
endif()