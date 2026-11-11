/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include <memory>

namespace sol
{
    class state;
}

namespace rhodium
{

    class LuaRuntime
    {
    public:
        LuaRuntime();
        ~LuaRuntime();

        sol::state &state();

    private:
        std::unique_ptr<sol::state> m_state;
    };

} // namespace rhodium
