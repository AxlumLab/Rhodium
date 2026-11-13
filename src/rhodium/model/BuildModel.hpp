/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/PackageDesc.hpp"
#include "rhodium/model/WorkspaceDesc.hpp"

#include <string>

namespace rhodium
{

    class DiagnosticSink;

    struct BuildModel
    {
        WorkspaceDesc workspace;
        PackageDesc rootPackage;
        std::string manifestName = "rhodium.lua";
    };

    class RHODIUM_EXPORT ModelValidator
    {
    public:
        static bool validate(const BuildModel &model, DiagnosticSink &diagnostics);
    };

} // namespace rhodium
