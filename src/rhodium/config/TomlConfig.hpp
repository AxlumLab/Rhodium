/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace rhodium
{

    struct ToolConfig
    {
        std::string buildDir = ".rhodium/build";
        std::string defaultPlatform;
        std::string defaultConfig;
        std::string defaultArch;

        static std::optional<ToolConfig> load(const std::filesystem::path &path);
    };

} // namespace rhodium
