/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <string>

namespace rhodium
{
    struct BuildSelection
    {
        std::string config = "debug";
        std::string toolchainId;
    };

    class RHODIUM_EXPORT BuildConfig
    {
    public:
        static std::string normalize(const std::string &input, const std::string &fallback = {});
        static bool validate(const std::string &config, DiagnosticSink &diagnostics);
        static bool isDebug(const std::string &config);
        static bool isRelease(const std::string &config);
    };
}
