set(_rhodium_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/Rhodium")

install(TARGETS rhodium_core
    EXPORT RhodiumTargets
    RUNTIME_DEPENDENCY_SET RhodiumRuntimeDependencies
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Runtime NAMELINK_COMPONENT Development
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Development
)
if(RHODIUM_BUILD_CLI)
    install(TARGETS rhodium
        RUNTIME_DEPENDENCY_SET RhodiumRuntimeDependencies
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime
    )
endif()

# CLI registration and command orchestration are not part of the library SDK.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/rhodium/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rhodium"
    COMPONENT Development
    FILES_MATCHING PATTERN "*.hpp" PATTERN "cli" EXCLUDE
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/rhodium/export.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rhodium" COMPONENT Development
)
install(EXPORT RhodiumTargets
    FILE RhodiumTargets.cmake
    NAMESPACE Rhodium::
    DESTINATION "${_rhodium_package_dir}" COMPONENT Development
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RhodiumConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RhodiumConfig.cmake"
    INSTALL_DESTINATION "${_rhodium_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RhodiumConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RhodiumConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RhodiumConfigVersion.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RhodiumLua.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RhodiumArchive.cmake"
    DESTINATION "${_rhodium_package_dir}" COMPONENT Development
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/Rhodium" COMPONENT Runtime
)

# Windows DLL consumers and the installed CLI need the dependent DLLs beside them.
if(WIN32 AND (BUILD_SHARED_LIBS OR RHODIUM_BUILD_CLI))
    set(_rhodium_runtime_dirs "$<TARGET_FILE_DIR:rhodium_core>")
    if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
        list(APPEND _rhodium_runtime_dirs
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<IF:$<CONFIG:Debug>,debug/bin,bin>"
        )
    endif()
    install(CODE [[
        if(POLICY CMP0207)
            cmake_policy(SET CMP0207 NEW)
        endif()
    ]] COMPONENT Runtime)
    install(RUNTIME_DEPENDENCY_SET RhodiumRuntimeDependencies
        DIRECTORIES ${_rhodium_runtime_dirs}
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES [[.*[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\].*]]
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime
    )
endif()
