/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */
#pragma once

#include "rhodium/export.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/model/DependencyOverride.hpp"
#include "rhodium/resolve/ResolvedDependencyArtifacts.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace rhodium
{

    class DiagnosticSink;

    class RHODIUM_EXPORT PackageArtifactResolver
    {
    public:
        bool load(const LockFile &lockFile, const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics);
        bool load(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                  const DependencyOverrideSet &overrides, DiagnosticSink &diagnostics);
        bool load(const LockFile &lockFile, const std::filesystem::path &workspaceRoot, const BuildSelection &selection,
                  const DependencyOverrideSet &overrides, DiagnosticSink &diagnostics);
        const ResolvedDependencyArtifact *find(const std::string &name) const;
        std::vector<ResolvedDependencyArtifact> packages() const;

    private:
        static bool shouldLoad(const LockedPackage &locked, const std::string &config, const std::string &toolchainId,
                               const DependencyOverrideSet &overrides);
        static void appendDirs(std::vector<std::filesystem::path> &output, const std::filesystem::path &root,
                               const std::vector<std::string> &values);

        std::unordered_map<std::string, ResolvedDependencyArtifact> packages_;
    };

} // namespace rhodium
