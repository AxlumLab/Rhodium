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

    class RHODIUM_EXPORT Environment
    {
    public:
        static std::string hostPlatformName();
        static std::string compilerName();
        static std::string buildModeName();
    };

} // namespace rhodium
