file(GLOB RHODIUM_CLI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/cli/*.cpp"
)
file(GLOB RHODIUM_CMAKE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/cmake/*.cpp"
)
file(GLOB RHODIUM_CONFIG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/config/*.cpp"
)
file(GLOB RHODIUM_CORE_MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/core/*.cpp"
)
file(GLOB RHODIUM_DIAGNOSTICS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/diagnostics/*.cpp"
)
file(GLOB RHODIUM_MODEL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/model/*.cpp"
)
file(GLOB RHODIUM_PACKAGE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/package/*.cpp"
)
file(GLOB RHODIUM_RESOLVE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/resolve/*.cpp"
)
file(GLOB RHODIUM_SCRIPT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/script/*.cpp"
)
file(GLOB RHODIUM_TOOLCHAIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/toolchain/*.cpp"
)
file(GLOB RHODIUM_UTILS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/utils/*.cpp"
)

set(RHODIUM_CORE_SOURCES
    ${RHODIUM_CMAKE_SOURCES}
    ${RHODIUM_CONFIG_SOURCES}
    ${RHODIUM_CORE_MODULE_SOURCES}
    ${RHODIUM_DIAGNOSTICS_SOURCES}
    ${RHODIUM_MODEL_SOURCES}
    ${RHODIUM_PACKAGE_SOURCES}
    ${RHODIUM_RESOLVE_SOURCES}
    ${RHODIUM_SCRIPT_SOURCES}
    ${RHODIUM_TOOLCHAIN_SOURCES}
    ${RHODIUM_UTILS_SOURCES}
)

add_library(rhodium_core ${RHODIUM_CORE_SOURCES})
add_library(Rhodium::Core ALIAS rhodium_core)
if(WIN32)
    target_link_libraries(rhodium_core PRIVATE advapi32)
endif()
set_target_properties(rhodium_core PROPERTIES
    EXPORT_NAME Core
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    DEBUG_POSTFIX d
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    MSVC_RUNTIME_LIBRARY "${RHODIUM_MSVC_RUNTIME_LIBRARY}"
)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/rhodium")
generate_export_header(rhodium_core
    BASE_NAME RHODIUM
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/generated/rhodium/export.hpp"
)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(rhodium_core PUBLIC RHODIUM_STATIC_DEFINE)
endif()
target_include_directories(rhodium_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(rhodium_core PUBLIC cxx_std_20)
target_compile_definitions(rhodium_core PUBLIC RHODIUM_VERSION=\"${PROJECT_VERSION}\")
rhodium_link_core_dependencies(rhodium_core)

if(MSVC)
    target_compile_options(rhodium_core PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(rhodium_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(RHODIUM_BUILD_CLI OR RHODIUM_BUILD_TESTS)
    add_library(rhodium_cli STATIC ${RHODIUM_CLI_SOURCES})
    target_link_libraries(rhodium_cli PUBLIC Rhodium::Core PRIVATE
        CLI11::CLI11 pugixml::pugixml CURL::libcurl LibArchive::LibArchive ${RHODIUM_GIT_TARGET}
    )
    set_target_properties(rhodium_cli PROPERTIES
        MSVC_RUNTIME_LIBRARY "${RHODIUM_MSVC_RUNTIME_LIBRARY}"
    )
    if(MSVC)
        target_compile_options(rhodium_cli PRIVATE /utf-8 /W4 /permissive-)
    else()
        target_compile_options(rhodium_cli PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(RHODIUM_BUILD_CLI)
    add_executable(rhodium "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/main.cpp")
    target_link_libraries(rhodium PRIVATE rhodium_cli)
    set_target_properties(rhodium PROPERTIES
        OUTPUT_NAME rhodium
        MSVC_RUNTIME_LIBRARY "${RHODIUM_MSVC_RUNTIME_LIBRARY}"
    )
    if(APPLE)
        set_property(TARGET rhodium PROPERTY INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
    elseif(UNIX)
        set_property(TARGET rhodium PROPERTY INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    endif()
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/main.cpp"
    ${RHODIUM_CORE_SOURCES}
    ${RHODIUM_CLI_SOURCES}
)
