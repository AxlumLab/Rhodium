/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/package/PackageBuilderUtil.hpp"

#include <map>
#include <string>
#include <vector>

namespace rhodium
{
    class RHODIUM_EXPORT BuildVar
    {
    public:
        static std::string expand(std::string value, const BuildUtil::Context &context);
        static std::vector<std::string> expand(const std::vector<std::string> &values, const BuildUtil::Context &context);
        static std::map<std::string, std::string> expand(const std::map<std::string, std::string> &values, const BuildUtil::Context &context);

    private:
        static void replace(std::string &value, const std::string &from, const std::string &to);
    };
} // namespace rhodium
