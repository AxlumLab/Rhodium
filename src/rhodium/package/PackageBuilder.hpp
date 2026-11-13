/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/PackageArtifact.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    class DiagnosticSink;
    class ToolchainSnapshot;

    struct PackageBuildOptions
    {
        bool runBuild = true;
        bool runExport = true;
        ToolchainSnapshot *toolchains = nullptr;
    };

    struct PackageBuildRequest
    {
        const DependencyDesc &dependency;
        std::filesystem::path sourceDir;
        std::filesystem::path buildDir;
        std::filesystem::path artifactDir;
        std::string commit;
        std::filesystem::path workspaceRoot;
        std::string preferredToolchain;
        std::string config;
        PackageBuildOptions options;
    };

    class RHODIUM_EXPORT PackageBuilder
    {
    public:
        virtual ~PackageBuilder() = default;
        virtual bool build(const PackageBuildRequest &request, PackageArtifact &artifact,
                           DiagnosticSink &diagnostics) const = 0;
    };

} // namespace rhodium
