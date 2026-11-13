/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/resolve/ResolvedDependencyArtifacts.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    struct CMakeDependencyExportMetadata
    {
        std::string toolchainId;
        std::string compilerKind;
        std::string config;
        std::string arch;
        std::string runtime;
        std::string linkage;
    };

    class RHODIUM_EXPORT CMakeDependency
    {
    public:
        static bool writeFiles(const std::vector<ResolvedDependencyArtifact> &packages,
                                       const std::filesystem::path &outputDir,
                                       const CMakeDependencyExportMetadata &metadata,
                                       DiagnosticSink &diagnostics);
        static bool writeFiles(const std::vector<ResolvedDependencyArtifact> &packages,
                                       const std::filesystem::path &outputDir,
                                       DiagnosticSink &diagnostics);
    };

}
