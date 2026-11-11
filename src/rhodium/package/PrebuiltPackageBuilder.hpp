/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/package/PackageBuilder.hpp"

#include <vector>

namespace rhodium
{

    class PrebuiltPackageBuilder final : public PackageBuilder
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
        static bool dirs(const std::filesystem::path &root,
                         const std::vector<std::string> &dirs,
                         const std::string &field,
                         DiagnosticSink &diagnostics);
    };

} // namespace rhodium
