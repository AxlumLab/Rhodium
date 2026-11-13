/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <string>

namespace rhodium
{
    class RHODIUM_EXPORT StringUtils
    {
    public:
        static std::string toLower(std::string value);
        static bool equalsIgnoreCase(const std::string &lhs, const std::string &rhs);
    };
} // namespace rhodium
