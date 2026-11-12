/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/package/PackageBuilder.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <vector>

namespace rhodium
{
    class RHODIUM_EXPORT CustomPackageBuilder final : public PackageBuilder
    {
    public:
        bool build(const PackageBuildRequest &request, PackageArtifact &artifact,
                   DiagnosticSink &diagnostics) const override;

    private:
        static std::string listing(const std::filesystem::path &path);
        static bool run(const std::vector<std::string> &commands, const std::string &phase,
                        const BuildUtil::Context &context, DiagnosticSink &diagnostics);
    };

} // namespace rhodium
