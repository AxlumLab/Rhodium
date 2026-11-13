# @Author: AxlumLab
# @LastEditors: AxlumLab
# Copyright (c) 2026 by AxlumLab, All Rights Reserved.

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(LibArchive REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/RhodiumArchive.cmake")
find_package(zstd CONFIG REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/RhodiumLua.cmake")

find_package(libgit2 CONFIG QUIET)
if(TARGET libgit2::libgit2)
    set(RHODIUM_GIT_PACKAGE libgit2)
    set(RHODIUM_GIT_TARGET libgit2::libgit2)
elseif(TARGET libgit2::libgit2package)
    set(RHODIUM_GIT_PACKAGE libgit2)
    set(RHODIUM_GIT_TARGET libgit2::libgit2package)
else()
    find_package(unofficial-libgit2 CONFIG REQUIRED)
    set(RHODIUM_GIT_PACKAGE unofficial-libgit2)
    set(RHODIUM_GIT_TARGET unofficial::libgit2::libgit2)
endif()

find_package(xxHash CONFIG QUIET)
if(TARGET xxHash::xxhash)
    set(RHODIUM_XXHASH_PACKAGE xxHash)
    set(RHODIUM_XXHASH_TARGET xxHash::xxhash)
elseif(TARGET xxhash)
    set(RHODIUM_XXHASH_PACKAGE xxHash)
    set(RHODIUM_XXHASH_TARGET xxhash)
else()
    find_package(unofficial-xxhash CONFIG REQUIRED)
    set(RHODIUM_XXHASH_PACKAGE unofficial-xxhash)
    set(RHODIUM_XXHASH_TARGET unofficial::xxhash::xxhash)
endif()

if(TARGET zstd::libzstd_shared)
    set(RHODIUM_ZSTD_TARGET zstd::libzstd_shared)
elseif(TARGET zstd::libzstd_static)
    set(RHODIUM_ZSTD_TARGET zstd::libzstd_static)
elseif(TARGET zstd::zstd)
    set(RHODIUM_ZSTD_TARGET zstd::zstd)
else()
    message(FATAL_ERROR "No supported zstd target was found")
endif()

if(RHODIUM_BUILD_CLI OR RHODIUM_BUILD_TESTS)
    find_package(CLI11 CONFIG REQUIRED)
    find_package(pugixml CONFIG REQUIRED)
endif()

function(rhodium_link_core_dependencies target_name)
    target_link_libraries(${target_name}
        PUBLIC sol2::sol2 fmt::fmt nlohmann_json::nlohmann_json tomlplusplus::tomlplusplus Lua::Lua
        PRIVATE spdlog::spdlog CURL::libcurl LibArchive::LibArchive
                ${RHODIUM_GIT_TARGET} ${RHODIUM_XXHASH_TARGET} ${RHODIUM_ZSTD_TARGET}
    )
endfunction()
