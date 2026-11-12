# FindLibArchive exposes a single location. vcpkg installs both CRT variants;
# select the matching library for each multi-config build configuration.
if(WIN32 AND TARGET LibArchive::LibArchive AND VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
    find_library(_rhodium_archive_release NAMES archive libarchive NO_CACHE
        PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH)
    find_library(_rhodium_archive_debug NAMES archive libarchive NO_CACHE
        PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib" NO_DEFAULT_PATH)
    if(_rhodium_archive_release AND _rhodium_archive_debug)
        set_target_properties(LibArchive::LibArchive PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION "${_rhodium_archive_release}"
            IMPORTED_LOCATION_RELEASE "${_rhodium_archive_release}"
            IMPORTED_LOCATION_DEBUG "${_rhodium_archive_debug}"
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            MAP_IMPORTED_CONFIG_MINSIZEREL Release
        )
    endif()
    unset(_rhodium_archive_release)
    unset(_rhodium_archive_debug)
endif()
