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

    std::optional<sol::table> findPlatformTable(sol::table options, const char *field, const std::string &activePlatform);
    std::optional<sol::table> findPlatformOverlay(sol::table options, const std::string &activePlatform);
    std::optional<sol::table> findCustomCompileOverlay(sol::table options, const std::string &activePlatform);
    void applyDependencyOverlays(sol::table options, DependencyDesc &dependency, const std::string &activePlatform);

} // namespace rhodium
