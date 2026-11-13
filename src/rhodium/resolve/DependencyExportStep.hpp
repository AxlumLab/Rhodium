/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/config/BuildConfig.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    class DiagnosticSink;
    struct PackageArtifact;

    class RHODIUM_EXPORT DepExport
    {
    public:
        static bool write(PackageArtifact &artifact, LockedPackage &updated, const std::filesystem::path &artifactDir,
                          DiagnosticSink &diagnostics);
        static bool reusable(const LockedPackage &locked, const LockedPackage &expected,
                             const std::filesystem::path &artifactDir);
        static bool gc(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                       const BuildSelection &selection, const std::string &packageName, DiagnosticSink &diagnostics);

    private:
        static bool artifactsEqual(const DependencyArtifacts &left, const DependencyArtifacts &right);
    };

} // namespace rhodium
