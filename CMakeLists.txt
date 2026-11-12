cmake_minimum_required(VERSION 3.25)

option(BUILD_SHARED_LIBS "Build Rhodium as a shared library" OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(cmake/RhodiumVcpkg.cmake)
endif()

project(Rhodium VERSION 0.1.2 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

option(RHODIUM_BUILD_CLI "Build the rhodium command-line executable" ${PROJECT_IS_TOP_LEVEL})
option(RHODIUM_INSTALL "Generate Rhodium installation rules" ${PROJECT_IS_TOP_LEVEL})

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    set(_rhodium_test_default ${BUILD_TESTING})
else()
    set(_rhodium_test_default OFF)
endif()
option(RHODIUM_BUILD_TESTS "Build the existing Rhodium tests" ${_rhodium_test_default})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(RHODIUM_MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
elseif(VCPKG_TARGET_TRIPLET MATCHES "-static$")
    set(RHODIUM_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(RHODIUM_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(MSVC AND BUILD_SHARED_LIBS AND NOT RHODIUM_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
    message(FATAL_ERROR
        "Shared Rhodium exchanges C++ objects with its caller and requires the DLL CRT. "
        "Use a dynamic-CRT triplet such as x64-windows or x64-windows-static-md, "
        "and a matching CMAKE_MSVC_RUNTIME_LIBRARY.")
endif()

include(cmake/RhodiumMsvcNinjaSdk.cmake)
include(cmake/RhodiumDependencies.cmake)
include(cmake/RhodiumTargets.cmake)

if(RHODIUM_BUILD_TESTS)
    enable_testing()
    include(cmake/RhodiumTests.cmake)
endif()

if(RHODIUM_INSTALL)
    include(cmake/RhodiumInstall.cmake)
endif()
