cmake_minimum_required(VERSION 3.25)

include(cmake/RhodiumVcpkg.cmake)

project(Rhodium VERSION 0.1.2 LANGUAGES C CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/RhodiumMsvcNinjaSdk.cmake)
include(cmake/RhodiumDependencies.cmake)
include(cmake/RhodiumTargets.cmake)

include(CTest)
if(BUILD_TESTING)
    include(cmake/RhodiumTests.cmake)
endif()
