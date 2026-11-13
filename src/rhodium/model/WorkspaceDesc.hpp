/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include <string>

namespace rhodium
{
    struct WorkspaceDesc
    {
        std::string name;
        std::string cppStandard;
        std::string defaultConfig;
        std::string defaultPlatform;
        std::string defaultArch;
        bool generateCMakeUserPresets = false;
    };

} // namespace rhodium
