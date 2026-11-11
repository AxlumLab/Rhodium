if(NOT MSVC)
    return()
endif()

function(_rhodium_first_existing out_var)
    foreach(_candidate IN LISTS ARGN)
        if(EXISTS "${_candidate}")
            set(${out_var} "${_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_rhodium_latest_child out_var parent)
    if(NOT EXISTS "${parent}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    file(GLOB _children LIST_DIRECTORIES true "${parent}/*")
    list(SORT _children COMPARE NATURAL ORDER DESCENDING)
    foreach(_child IN LISTS _children)
        if(IS_DIRECTORY "${_child}")
            set(${out_var} "${_child}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_rhodium_msvc_host_arch out_var)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _processor)
    if(_processor MATCHES "arm64|aarch64")
        set(${out_var} "arm64" PARENT_SCOPE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(${out_var} "x64" PARENT_SCOPE)
    else()
        set(${out_var} "x86" PARENT_SCOPE)
    endif()
endfunction()

function(_rhodium_enable_msvc_ninja_sdk_paths)
    cmake_path(CONVERT "${CMAKE_CXX_COMPILER}" TO_CMAKE_PATH_LIST _compiler)
    if(NOT _compiler MATCHES "/VC/Tools/MSVC/[^/]+/bin/Host[^/]+/[^/]+/cl(\\.exe)?$")
        return()
    endif()

    string(REGEX REPLACE "/bin/Host[^/]+/[^/]+/cl(\\.exe)?$" "" _vc_tools_root "${_compiler}")
    if(NOT EXISTS "${_vc_tools_root}/include/string")
        return()
    endif()

    _rhodium_first_existing(_kits_root
        "C:/Windows Kits/10"
        "C:/Program Files (x86)/Windows Kits/10"
        "C:/Program Files/Windows Kits/10"
    )
    _rhodium_latest_child(_kits_version_root "${_kits_root}/Include")
    if(_kits_version_root)
        get_filename_component(_kits_version "${_kits_version_root}" NAME)
    endif()

    _rhodium_msvc_host_arch(_arch)

    set(_include_dirs
        "${_vc_tools_root}/include"
    )
    set(_lib_dirs
        "${_vc_tools_root}/lib/${_arch}"
    )

    if(_kits_version)
        foreach(_component IN ITEMS ucrt shared um winrt cppwinrt)
            if(EXISTS "${_kits_root}/Include/${_kits_version}/${_component}")
                list(APPEND _include_dirs "${_kits_root}/Include/${_kits_version}/${_component}")
            endif()
        endforeach()
        foreach(_component IN ITEMS ucrt um)
            if(EXISTS "${_kits_root}/Lib/${_kits_version}/${_component}/${_arch}")
                list(APPEND _lib_dirs "${_kits_root}/Lib/${_kits_version}/${_component}/${_arch}")
            endif()
        endforeach()
    endif()

    include_directories(BEFORE SYSTEM ${_include_dirs})
    link_directories(${_lib_dirs})

    message(STATUS "Rhodium: added MSVC SDK include paths for Ninja from ${_vc_tools_root}")
    if(_kits_version)
        message(STATUS "Rhodium: added Windows SDK ${_kits_version} paths from ${_kits_root}")
    endif()
endfunction()

_rhodium_enable_msvc_ninja_sdk_paths()
