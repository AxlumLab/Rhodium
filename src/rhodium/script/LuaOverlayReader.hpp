/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/model/DependencyDesc.hpp"

#include <sol/sol.hpp>

#include <optional>
#include <string>

namespace rhodium
{

    class LuaOverlayReader
    {
    public:
        static std::optional<sol::table> table(sol::table options, const char *field,
                                               const std::string &activePlatform);
        static std::optional<sol::table> platform(sol::table options, const std::string &activePlatform);
        static std::optional<sol::table> custom(sol::table options, const std::string &activePlatform);
        static void apply(sol::table options, DependencyDesc &dependency, const std::string &activePlatform);
    };

} // namespace rhodium
