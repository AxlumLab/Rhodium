/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <cstddef>
#include <string>

namespace rhodium
{
    class RHODIUM_EXPORT Hash
    {
    public:
        static std::string xxhash64Hex(const void *data, std::size_t size);
        static std::string xxhash64Hex(const std::string &value);
    };
} // namespace rhodium
