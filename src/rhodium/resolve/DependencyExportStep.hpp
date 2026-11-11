#pragma once

#include "rhodium/config/LockFile.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    class DiagnosticSink;
    struct PackageArtifact;

    class DepExport
    {
    public:
        static bool write(PackageArtifact &artifact,
                          LockedPackage &updated,
                          const std::filesystem::path &artifactDir,
                          DiagnosticSink &diagnostics);
        static bool reusable(const LockedPackage &locked,
                             const LockedPackage &expected,
                             const std::filesystem::path &artifactDir);
        static bool gc(const LockFile &lockFile,
                       const std::filesystem::path &workspaceRoot,
                       const std::string &config,
                       const std::string &toolchainId,
                       const std::string &packageName,
                       DiagnosticSink &diagnostics);

    private:
        static bool artifactsEqual(const DependencyArtifacts &left, const DependencyArtifacts &right);
    };

} // namespace rhodium
