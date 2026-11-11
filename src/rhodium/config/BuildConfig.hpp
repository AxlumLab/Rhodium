/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <string>

namespace rhodium
{
    class BuildConfig
    {
    public:
        static std::string normalize(const std::string &input, const std::string &fallback = {});
        static bool validate(const std::string &config, DiagnosticSink &diagnostics);
        static bool isDebug(const std::string &config);
        static bool isRelease(const std::string &config);
    };
}
