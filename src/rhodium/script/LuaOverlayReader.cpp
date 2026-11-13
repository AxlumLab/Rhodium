/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/script/LuaOverlayReader.hpp"

#include "rhodium/script/LuaDependencyReader.hpp"
#include "rhodium/script/LuaValueReader.hpp"

#include <vector>

namespace rhodium
{

    std::optional<sol::table> LuaOverlayReader::table(sol::table options, const char *field,
                                                      const std::string &activePlatform)
    {
        sol::object platformsValue = options[field];
        if (!platformsValue.is<sol::table>())
        {
            return std::nullopt;
        }
        sol::table platforms = platformsValue.as<sol::table>();
        std::vector<std::string> keys{activePlatform};
        if (activePlatform == "macos")
        {
            keys.push_back("mac");
        }
        if (activePlatform == "linux" || activePlatform == "macos")
        {
            keys.push_back("unix");
        }
        for (const auto &key : keys)
        {
            sol::object value = platforms[key];
            if (value.is<sol::table>())
            {
                return value.as<sol::table>();
            }
        }
        return std::nullopt;
    }

    std::optional<sol::table> LuaOverlayReader::platform(sol::table options, const std::string &activePlatform)
    {
        return LuaOverlayReader::table(options, "platforms", activePlatform);
    }

    std::optional<sol::table> LuaOverlayReader::custom(sol::table options, const std::string &activePlatform)
    {
        return LuaOverlayReader::table(options, "custom_compile", activePlatform);
    }

    void LuaOverlayReader::apply(sol::table options, DependencyDesc &dependency, const std::string &activePlatform)
    {
        if (auto overlay = LuaOverlayReader::platform(options, activePlatform))
        {
            LuaDependencyReader::overlay(*overlay, dependency);
            LuaValueReader::setString(*overlay, "sha256", dependency.sha256);
            if (LuaValueReader::has(*overlay, "strip_components"))
            {
                dependency.stripComponents =
                    LuaValueReader::integer(*overlay, "strip_components", dependency.stripComponents);
            }
            LuaValueReader::setArray(*overlay, "patches", dependency.patches);
        }
        if (auto customCompile = LuaOverlayReader::custom(options, activePlatform))
        {
            dependency.buildType = "custom";
            LuaDependencyReader::overlay(*customCompile, dependency);
        }
    }

} // namespace rhodium
