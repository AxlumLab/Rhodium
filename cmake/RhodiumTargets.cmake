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
    ${RHODIUM_CLI_SOURCES}
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

add_library(rhodium_core STATIC ${RHODIUM_CORE_SOURCES})
target_include_directories(rhodium_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(rhodium_core PUBLIC RHODIUM_VERSION=\"${PROJECT_VERSION}\")
rhodium_link_core_dependencies(rhodium_core)

if(MSVC)
    target_compile_options(rhodium_core PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(rhodium_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(rhodium "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/main.cpp")
target_link_libraries(rhodium PRIVATE rhodium_core)
set_target_properties(rhodium PROPERTIES OUTPUT_NAME rhodium)

install(TARGETS rhodium
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/main.cpp"
    ${RHODIUM_CORE_SOURCES}
)
