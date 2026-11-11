/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/package/PackageBuilder.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <vector>

namespace rhodium
{

    class MakePackageBuilder final : public PackageBuilder
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

    private:
        static bool run(const std::filesystem::path &workingDir,
                        const MakeBuildConfig &make,
                        const BuildUtil::Context &context,
                        const std::vector<std::string> &extraArgs,
                        const std::vector<std::string> &targets,
                        DiagnosticSink &diagnostics);
    };

} // namespace rhodium
