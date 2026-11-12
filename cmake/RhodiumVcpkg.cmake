set(RHODIUM_VCPKG_ROOT "" CACHE PATH "Path to the vcpkg root directory.")
if(NOT DEFINED VCPKG_TARGET_TRIPLET OR VCPKG_TARGET_TRIPLET STREQUAL "")
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "ARM64|arm64|aarch64")
            set(_rhodium_vcpkg_arch arm64)
        else()
            set(_rhodium_vcpkg_arch x64)
        endif()
        if(BUILD_SHARED_LIBS)
            set(VCPKG_TARGET_TRIPLET "${_rhodium_vcpkg_arch}-windows" CACHE STRING "vcpkg target triplet" FORCE)
        else()
            set(VCPKG_TARGET_TRIPLET "${_rhodium_vcpkg_arch}-windows-static" CACHE STRING "vcpkg target triplet" FORCE)
        endif()
        message(STATUS "Rhodium: defaulting vcpkg triplet to ${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

if(DEFINED VCPKG_TARGET_TRIPLET)
    set_property(CACHE VCPKG_TARGET_TRIPLET PROPERTY HELPSTRING "Optional vcpkg target triplet, such as x64-windows-static, x64-linux, arm64-osx. If unset, Rhodium selects a Windows triplet matching BUILD_SHARED_LIBS.")
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    set(_rhodium_vcpkg_candidates)

    if(RHODIUM_VCPKG_ROOT)
        list(APPEND _rhodium_vcpkg_candidates "${RHODIUM_VCPKG_ROOT}")
    endif()

    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        list(APPEND _rhodium_vcpkg_candidates "$ENV{VCPKG_ROOT}")
    endif()

    list(APPEND _rhodium_vcpkg_candidates
        "${CMAKE_CURRENT_LIST_DIR}/../third_party/vcpkg"
        "${CMAKE_CURRENT_LIST_DIR}/../extern/vcpkg"
        "${CMAKE_CURRENT_LIST_DIR}/../.vcpkg"
        "D:/Sdks/vcpkg"
        "C:/vcpkg"
    )

    foreach(_rhodium_vcpkg_root IN LISTS _rhodium_vcpkg_candidates)
        if(EXISTS "${_rhodium_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "${_rhodium_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain file" FORCE)
            set(RHODIUM_VCPKG_ROOT "${_rhodium_vcpkg_root}" CACHE PATH "Path to the vcpkg root directory." FORCE)
            message(STATUS "Rhodium: using vcpkg at ${RHODIUM_VCPKG_ROOT}")
            break()
        endif()
    endforeach()
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    message(FATAL_ERROR
        "Rhodium requires vcpkg manifest mode for third-party dependencies, but no vcpkg toolchain was found.\n"
        "Pass -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake, set -DRHODIUM_VCPKG_ROOT=<path-to-vcpkg>, or set VCPKG_ROOT.\n"
        "Recommended configure command: cmake -S . -B out/build/ninja-vcpkg -G Ninja -DRHODIUM_VCPKG_ROOT=<path-to-vcpkg>.\n"
        "Expected toolchain: <vcpkg-root>/scripts/buildsystems/vcpkg.cmake"
    )
endif()
