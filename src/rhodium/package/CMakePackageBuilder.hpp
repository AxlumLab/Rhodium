/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/package/PackageBuilder.hpp"

#include <map>
#include <vector>

namespace rhodium
{

    class CMakePackageBuilder final : public PackageBuilder
    {
    public:
        bool build(const PackageBuildRequest &request, PackageArtifact &artifact,
                   DiagnosticSink &diagnostics) const override;

    private:
        class Stages;
        static bool run(const std::string &cmake, const std::vector<std::string> &args,
                        const std::map<std::string, std::string> &environment, DiagnosticSink &diagnostics);
    };

} // namespace rhodium
