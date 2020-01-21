# Install for windows
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

#https://stackoverflow.com/questions/20433308/cmake-on-windows-doesnt-add-shared-library-paths-works-on-linux

# install for other platforms
else()

    # sudo make install
    install(TARGETS radrpc_static radrpc_shared DESTINATION "${INSTALL_LIB_DIR}")
    install(
        DIRECTORY "${INSTALL_HEADER_DIR}"
        DESTINATION "include"
        FILES_MATCHING
        PATTERN "*.hpp")

endif()





#[[install(TARGETS radrpc_static EXPORT RadrpcConfig DESTINATION "${INSTALL_LIB_DIR}")
install(TARGETS radrpc_shared EXPORT RadrpcConfig DESTINATION "${INSTALL_LIB_DIR}")
install(EXPORT RadrpcConfig DESTINATION "${INSTALL_LIB_DIR}")

export(
    EXPORT radrpc
    TARGETS radrpc_static radrpc_shared
    NAMESPACE radrpc::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/RadrpcConfig.cmake"
)]]

#[[install(EXPORT
        RadrpcConfig
    DESTINATION "${CMAKE_INSTALL_DATADIR}/RadrpcConfig/cmake"
    NAMESPACE radrpc::
)]]





install(TARGETS radrpc_static EXPORT RadrpcConfig DESTINATION "${INSTALL_LIB_DIR}")
install(TARGETS radrpc_shared EXPORT RadrpcConfig DESTINATION "${INSTALL_LIB_DIR}")
export(
    TARGETS radrpc_static radrpc_shared
    NAMESPACE radrpc::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/RadrpcConfig.cmake"
)
install(EXPORT
        RadrpcConfig
    DESTINATION "${INSTALL_LIB_DIR}/cmake"
    NAMESPACE radrpc::
)


