# @Author: AxlumLab
# @LastEditors: AxlumLab
# Copyright (c) 2026 by AxlumLab, All Rights Reserved.

# FindLua does not provide Lua::Lua on every supported CMake version.
# Use a rediscoverable target instead of embedding build-machine paths in the export.
if(NOT TARGET Lua::Lua)
    add_library(Lua::Lua INTERFACE IMPORTED GLOBAL)
    set_target_properties(Lua::Lua PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
    )
    target_link_libraries(Lua::Lua INTERFACE ${LUA_LIBRARIES})
endif()
