/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/export.hpp"

#include <string>

namespace rhodium
{

    class RHODIUM_EXPORT LuaCondition
    {
    public:
        static bool matches(const std::string &condition, const std::string &activePlatform);
        static bool valid(const std::string &condition);
    };

} // namespace rhodium
