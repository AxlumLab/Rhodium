/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/Hash.hpp"

#include <xxhash.h>
#include <fmt/format.h>

namespace rhodium
{
    std::string Hash::xxhash64Hex(const void *data, std::size_t size)
    {
        return fmt::format("{:016x}", XXH64(data, size, 0));
    }

    std::string Hash::xxhash64Hex(const std::string &value)
    {
        return xxhash64Hex(value.data(), value.size());
    }

} // namespace rhodium
