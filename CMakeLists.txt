cmake_minimum_required (VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json" DESTINATION "${PROJECT_SOURCE_DIR}")
endif()
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
set(CMAKE_CXX_STANDARD 14)
set(REPORT_DIR "Reports")
set(EXAMPLE_DIR "${PROJECT_SOURCE_DIR}/examples")
set(TEST_DIR "${PROJECT_SOURCE_DIR}/test")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/include/radrpc")
set(SANITIZER_SUPPRESSION "${PROJECT_SOURCE_DIR}/sanitizer_suppression.txt")
set(CACHE_OPTION OFF)

# Todo proper naming of global variables 
# for easier refactoring with an IDE & distinguish
# cmake variables
# maybe it is enough to prepend _G_
set(LOCAL "${PROJECT_SOURCE_DIR}/cmake")
include(ExternalProject)

include("${LOCAL}/functions/AddTestExecutable.cmake")
include("${LOCAL}/functions/BinaryWrapper.cmake")
include("${LOCAL}/functions/SetDefinitions.cmake")
include("${LOCAL}/functions/SetOption.cmake")

include("${LOCAL}/Option.cmake")
include("${LOCAL}/Package.cmake")
include("${LOCAL}/Flags.cmake")
include("${LOCAL}/Include.cmake")
include("${LOCAL}/Sanitizer.cmake")

include("${LOCAL}/binaries/Radrpc.cmake")
include("${LOCAL}/binaries/Test.cmake")
include("${LOCAL}/binaries/CatchMain.cmake")
include("${LOCAL}/binaries/ExampleAdvancedClient.cmake")
include("${LOCAL}/binaries/ExampleAdvancedServer.cmake")
include("${LOCAL}/binaries/ExampleClient.cmake")
include("${LOCAL}/binaries/ExampleServer.cmake")
include("${LOCAL}/binaries/ExampleSSLClient.cmake")
include("${LOCAL}/binaries/ExampleSSLServer.cmake")
include("${LOCAL}/binaries/StressClient.cmake")
include("${LOCAL}/binaries/StressServer.cmake")
include("${LOCAL}/binaries/UnitClient.cmake")
include("${LOCAL}/binaries/UnitMisc.cmake")
include("${LOCAL}/binaries/UnitSelf.cmake")
include("${LOCAL}/binaries/UnitServer.cmake")

include("${LOCAL}/WrapBinaries.cmake")