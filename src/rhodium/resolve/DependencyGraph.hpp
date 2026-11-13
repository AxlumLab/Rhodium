/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/BuildModel.hpp"

#include <vector>

namespace rhodium
{

    class DiagnosticSink;

    class RHODIUM_EXPORT DepGraph
    {
    public:
        static bool build(const BuildModel &model,
                          std::vector<DependencyDesc> &orderedDependencies,
                          DiagnosticSink &diagnostics);
    };

} // namespace rhodium
