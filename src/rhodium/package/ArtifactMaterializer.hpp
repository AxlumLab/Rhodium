/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
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
    class RHODIUM_EXPORT ArtifactFiles
    {
    public:
        static bool materialize(const BuildUtil::Context &context,
                                const DependencyArtifacts &requested,
                                DependencyArtifacts &exported,
                                DiagnosticSink &diagnostics);

    private:
        static std::filesystem::path input(const BuildUtil::Context &context, const std::string &value);
        static std::string output(const std::string &value);
        static bool copy(const BuildUtil::Context &context,
                         const std::vector<std::string> &requested,
                         std::vector<std::string> &exported,
                         const char *field,
                         DiagnosticSink &diagnostics);
    };
} // namespace rhodium
