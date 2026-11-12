/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

namespace rhodium
{
    class DiagnosticSink;
}

namespace rhodium
{
    class RHODIUM_EXPORT ArtifactCheck
    {
    public:
        static bool validate(const BuildUtil::Context &context,
                             const DependencyArtifacts &artifacts,
                             DiagnosticSink &diagnostics);

    private:
        static bool dirs(const BuildUtil::Context &context,
                         const std::vector<std::string> &dirs,
                         const char *field,
                         DiagnosticSink &diagnostics);
        static std::vector<std::string> libCandidates(const std::string &library, const BuildUtil::Context &context);
        static bool libs(const BuildUtil::Context &context,
                         const DependencyArtifacts &artifacts,
                         DiagnosticSink &diagnostics);
        static bool files(const BuildUtil::Context &context,
                          const std::vector<std::string> &files,
                          const char *field,
                          DiagnosticSink &diagnostics);
    };
} // namespace rhodium
