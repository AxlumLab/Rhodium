/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include <filesystem>
#include <map>
#include <string>

namespace rhodium
{

    struct DependencyOverrideDesc
    {
        std::string name;
        std::string path;
    };

    struct DependencyOverride
    {
        std::string name;
        std::string rawPath;
        std::filesystem::path path;
        std::string source;
    };

    struct DependencyOverrideSet
    {
        std::map<std::string, DependencyOverride> overrides;
    };

} // namespace rhodium
