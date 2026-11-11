/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/model/BuildModel.hpp"
#include "rhodium/model/DependencyOverride.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace rhodium
{

    class DiagnosticSink;

    struct LocalDependencyOverrides
    {
        std::map<std::string, std::string> paths;
    };

    class DependencyOverrides
    {
    public:
        static std::filesystem::path path(const std::filesystem::path &workspaceRoot);
        static bool read(const std::filesystem::path &workspaceRoot, LocalDependencyOverrides &overrides, DiagnosticSink &diagnostics);
        static bool write(const std::filesystem::path &workspaceRoot, const LocalDependencyOverrides &overrides, DiagnosticSink &diagnostics);
        static bool make(const BuildModel &model,
                         const std::filesystem::path &workspaceRoot,
                         const std::vector<std::string> &cliOverrides,
                         DependencyOverrideSet &overrides,
                         DiagnosticSink &diagnostics);
    };

} // namespace rhodium
