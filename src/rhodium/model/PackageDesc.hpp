/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/model/DependencyOverride.hpp"

#include <string>
#include <vector>

namespace rhodium
{

    struct PackageDesc
    {
        std::string name;
        std::string version;
        std::vector<DependencyDesc> dependencies;
        std::vector<DependencyOverrideDesc> dependencyOverrides;
    };

} // namespace rhodium
