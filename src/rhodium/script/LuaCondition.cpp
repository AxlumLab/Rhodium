/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/script/LuaCondition.hpp"

#include "rhodium/core/StringUtil.hpp"

namespace rhodium
{

    bool LuaCondition::matches(const std::string &condition, const std::string &activePlatform)
    {
        const auto normalized = StringUtils::toLower(condition);
        if (normalized == "all" || normalized == "*")
        {
            return true;
        }
        if (normalized == "unix")
        {
            return activePlatform == "linux" || activePlatform == "macos";
        }
        if (normalized == "mac")
        {
            return activePlatform == "macos";
        }
        return normalized == activePlatform;
    }

    bool LuaCondition::valid(const std::string &condition)
    {
        const auto normalized = StringUtils::toLower(condition);
        return normalized == "all" || normalized == "*" || normalized == "unix" || normalized == "windows" ||
               normalized == "linux" || normalized == "mac" || normalized == "macos" || normalized == "android" ||
               normalized == "ios";
    }

} // namespace rhodium
