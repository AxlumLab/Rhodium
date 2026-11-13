/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/package/PackageBuilder.hpp"

#include <vector>

namespace rhodium
{

    class RHODIUM_EXPORT PrebuiltPackageBuilder final : public PackageBuilder
    {
    public:
        bool build(const PackageBuildRequest &request, PackageArtifact &artifact,
                   DiagnosticSink &diagnostics) const override;

    private:
        static bool dirs(const std::filesystem::path &root, const std::vector<std::string> &dirs,
                         const std::string &field, DiagnosticSink &diagnostics);
    };

} // namespace rhodium
