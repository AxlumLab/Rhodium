/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/script/LuaRuntime.hpp"

#include <sol/sol.hpp>

namespace rhodium
{

    LuaRuntime::LuaRuntime()
        : m_state(std::make_unique<sol::state>())
    {
        m_state->open_libraries(sol::lib::base, sol::lib::table, sol::lib::string, sol::lib::math);
    }

    LuaRuntime::~LuaRuntime() = default;

    sol::state &LuaRuntime::state()
    {
        return *m_state;
    }

} // namespace rhodium
