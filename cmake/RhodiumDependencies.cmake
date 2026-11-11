find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

find_package(libgit2 CONFIG QUIET)
if(NOT libgit2_FOUND)
    find_package(unofficial-libgit2 CONFIG REQUIRED)
endif()

find_package(CURL CONFIG REQUIRED)
find_package(xxHash CONFIG QUIET)
if(NOT xxHash_FOUND)
    find_package(unofficial-xxhash CONFIG REQUIRED)
endif()
find_package(zstd CONFIG REQUIRED)
find_package(LibArchive REQUIRED)

function(rhodium_link_core_dependencies target_name)
    target_link_libraries(${target_name} PUBLIC
        sol2::sol2
        CLI11::CLI11
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        tomlplusplus::tomlplusplus
        pugixml::pugixml
        CURL::libcurl
        LibArchive::LibArchive
    )

    if(TARGET Lua::Lua)
        target_link_libraries(${target_name} PUBLIC Lua::Lua)
    else()
        target_include_directories(${target_name} PUBLIC ${LUA_INCLUDE_DIR})
        target_link_libraries(${target_name} PUBLIC ${LUA_LIBRARIES})
    endif()

    if(TARGET libgit2::libgit2)
        target_link_libraries(${target_name} PUBLIC libgit2::libgit2)
    elseif(TARGET libgit2::libgit2package)
        target_link_libraries(${target_name} PUBLIC libgit2::libgit2package)
    elseif(TARGET unofficial::libgit2::libgit2)
        target_link_libraries(${target_name} PUBLIC unofficial::libgit2::libgit2)
    endif()

    if(TARGET xxHash::xxhash)
        target_link_libraries(${target_name} PUBLIC xxHash::xxhash)
    elseif(TARGET unofficial::xxhash::xxhash)
        target_link_libraries(${target_name} PUBLIC unofficial::xxhash::xxhash)
    elseif(TARGET xxhash)
        target_link_libraries(${target_name} PUBLIC xxhash)
    endif()

    if(TARGET zstd::libzstd_shared)
        target_link_libraries(${target_name} PUBLIC zstd::libzstd_shared)
    elseif(TARGET zstd::libzstd_static)
        target_link_libraries(${target_name} PUBLIC zstd::libzstd_static)
    elseif(TARGET zstd::zstd)
        target_link_libraries(${target_name} PUBLIC zstd::zstd)
    endif()
endfunction()
