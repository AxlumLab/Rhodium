/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <memory>

namespace sol
{
    class state;
}

namespace rhodium
{

    class RHODIUM_EXPORT LuaRuntime
    {
    public:
        LuaRuntime();
        ~LuaRuntime();

        sol::state &state();

    private:
        std::unique_ptr<sol::state> m_state;
    };

} // namespace rhodium
