/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/core/Environment.hpp"

namespace rhodium
{

    std::string Environment::hostPlatformName()
    {
#if defined(_WIN32)
        return "windows";
#elif defined(__APPLE__)
        return "macos";
#elif defined(__linux__)
        return "linux";
#else
        return "unknown";
#endif
    }

    std::string Environment::compilerName()
    {
#if defined(_MSC_VER)
        return "msvc";
#elif defined(__clang__)
        return "clang";
#elif defined(__GNUC__)
        return "gcc";
#else
        return "unknown";
#endif
    }

    std::string Environment::buildModeName()
    {
#if defined(NDEBUG)
        return "release";
#else
        return "debug";
#endif
    }

} // namespace rhodium
