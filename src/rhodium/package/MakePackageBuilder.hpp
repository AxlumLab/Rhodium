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
        bool build(const PackageBuildRequest &request, PackageArtifact &artifact,
                   DiagnosticSink &diagnostics) const override;

    private:
        static bool run(const MakeBuildConfig &make, const BuildUtil::Context &context,
                        const std::vector<std::string> &extraArgs, const std::vector<std::string> &targets,
                        DiagnosticSink &diagnostics);
    };

} // namespace rhodium
