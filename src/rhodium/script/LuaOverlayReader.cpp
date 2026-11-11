/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/script/LuaOverlayReader.hpp"

#include "rhodium/script/LuaDependencyReader.hpp"
#include "rhodium/script/LuaValueReader.hpp"

#include <vector>

namespace rhodium
{

    std::optional<sol::table> findPlatformTable(sol::table options, const char *field, const std::string &activePlatform)
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

    std::optional<sol::table> findPlatformOverlay(sol::table options, const std::string &activePlatform)
    {
        return findPlatformTable(options, "platforms", activePlatform);
    }

    std::optional<sol::table> findCustomCompileOverlay(sol::table options, const std::string &activePlatform)
    {
        return findPlatformTable(options, "custom_compile", activePlatform);
    }

    void applyDependencyOverlays(sol::table options, DependencyDesc &dependency, const std::string &activePlatform)
    {
        if (auto overlay = findPlatformOverlay(options, activePlatform))
        {
            overlayDependencyOptions(*overlay, dependency);
            setStringIfPresent(*overlay, "sha256", dependency.sha256);
            if (hasField(*overlay, "strip_components"))
            {
                dependency.stripComponents = getIntOr(*overlay, "strip_components", dependency.stripComponents);
            }
            setStringArrayIfPresent(*overlay, "patches", dependency.patches);
        }
        if (auto customCompile = findCustomCompileOverlay(options, activePlatform))
        {
            dependency.buildType = "custom";
            overlayDependencyOptions(*customCompile, dependency);
        }
    }

} // namespace rhodium
