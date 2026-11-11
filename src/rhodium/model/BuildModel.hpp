/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

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

    bool validateBuildModel(const BuildModel &model, DiagnosticSink &diagnostics);

} // namespace rhodium
