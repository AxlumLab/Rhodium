/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/config/BuildConfig.hpp"

#include "rhodium/core/StringUtil.hpp"

namespace rhodium
{
    std::string BuildConfig::normalize(const std::string &input, const std::string &fallback)
    {
        auto value = input.empty() ? fallback : input;
        value = StringUtils::toLower(value);
        return value.empty() ? "debug" : value;
    }

    bool BuildConfig::validate(const std::string &config, DiagnosticSink &diagnostics)
    {
        if (config == "debug" || config == "release")
        {
            return true;
        }
        diagnostics.error("config must be debug or release: " + config);
        return false;
    }

    bool BuildConfig::isDebug(const std::string &config)
    {
        return normalize(config) == "debug";
    }

    bool BuildConfig::isRelease(const std::string &config)
    {
        return normalize(config) == "release";
    }
}
