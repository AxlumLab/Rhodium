/*
 * @Author: khfahqp khfahqp@gmail.com
 * @LastEditors: khfahqp khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/config/LockFile.hpp"
#include <string>

namespace rhodium
{
    class PackageMetadata final
    {
    public:
        static std::string value(const LockFile &lockFile, const std::string &config, const std::string &toolchainId, const std::string LockedPackage::*field);
    };
}