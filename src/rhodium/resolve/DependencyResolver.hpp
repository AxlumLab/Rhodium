/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/BuildModel.hpp"
#include "rhodium/model/DependencyOverride.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    class DiagnosticSink;
    class ToolchainSnapshot;

    struct DependencyRestoreOptions
    {
        bool locked = false;
        bool updateAll = false;
        bool buildOnly = false;
        bool exportOnly = false;
        bool rebuild = false;
        std::string packageName;
        std::string updatePackage;
        std::string toolchainContentHash;
        ToolchainSnapshot *toolchains = nullptr;
    };

    struct DependencyRestoreRequest
    {
        std::string preferredToolchain;
        std::string config = "debug";
        DependencyOverrideSet overrides;
        DependencyRestoreOptions options;
    };

    class RHODIUM_EXPORT DependencyResolver
    {
    public:
        bool restore(const BuildModel &model, const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics,
                     const DependencyRestoreRequest &request = {}) const;
    };

} // namespace rhodium
