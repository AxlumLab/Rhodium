#pragma once

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

    class CMakeDependency
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
