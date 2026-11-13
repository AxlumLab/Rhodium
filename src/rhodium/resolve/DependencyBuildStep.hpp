/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/PackageArtifact.hpp"
#include "rhodium/package/PackageBuilder.hpp"
#include "rhodium/resolve/DependencyResolver.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace rhodium
{

    class DiagnosticSink;
    class PackageCache;

    class RHODIUM_EXPORT DepBuild
    {
    public:
        struct Context
        {
            const std::filesystem::path &workspaceRoot;
            PackageCache &cache;
            const std::string &effectiveConfig;
            const std::string &preferredToolchain;
            const DependencyRestoreOptions &options;
        };

        struct Result
        {
            PackageArtifact artifact;
            LockedPackage updated;
            std::filesystem::path artifactDir;
            bool skipPackage = false;
        };

        struct Request
        {
            const DependencyDesc &dependency;
            std::filesystem::path sourceRoot;
            std::string commit;
            LockedPackage *locked;
            bool wantsUpdate;
        };

        static bool run(const Request &request, const Context &context, Result &result, DiagnosticSink &diagnostics);

        static void applyConfig(DependencyDesc &dependency, const std::string &config);
        static std::string key(const std::string &config, const std::string &toolchainId);

    private:
        static std::unique_ptr<PackageBuilder> builder(const DependencyDesc &dependency);
        static std::filesystem::path subdir(const std::filesystem::path &source, const DependencyDesc &dependency);
        static bool removeBeforeRebuild(const std::filesystem::path &buildDir, DiagnosticSink &diagnostics);
        static std::string normalized(const std::filesystem::path &path);
        static bool hasCMakeCache(const std::filesystem::path &buildDir);
        static bool cacheMatchesDir(const std::filesystem::path &buildDir);
        static bool hasStage(const DependencyDesc &dependency);
        static bool completed(const std::filesystem::path &buildDir, const std::string &buildHash);
    };

} // namespace rhodium
