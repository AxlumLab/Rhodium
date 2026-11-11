/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/package/PackageBuilder.hpp"

namespace rhodium
{

    class ConfigureMakePackageBuilder final : public PackageBuilder
    {
    public:
        bool build(const DependencyDesc &dependency,
                   const std::filesystem::path &sourceDir,
                   const std::filesystem::path &buildDir,
                   const std::filesystem::path &artifactDir,
                   const std::string &commit,
                   const std::filesystem::path &workspaceRoot,
                   const std::string &preferredToolchain,
                   const std::string &config,
                   const PackageBuildOptions &options,
                   PackageArtifact &artifact,
                   DiagnosticSink &diagnostics) const override;
    };

} // namespace rhodium
