/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/ArtifactGc.hpp"
#include "rhodium/package/CacheReferences.hpp"
#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <filesystem>
#include <map>
#include <set>
#include <vector>

namespace rhodium
{
    bool ArtifactGc::runArtifacts(const LockFile &lockFile, const std::filesystem::path &workspaceRoot, bool apply,
                                  const std::string &packageFilter, DiagnosticSink &diagnostics)
    {
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return false;
        }

        std::set<std::string> referenced;
        for (const auto &package : lockFile.packages)
        {
            if (!package.artifactId.empty())
            {
                referenced.insert(package.artifactId);
            }
        }

        const auto artifactsRoot = Path::stateRoot(workspaceRoot) / "artifacts";
        if (!std::filesystem::is_directory(artifactsRoot) || std::filesystem::is_symlink(artifactsRoot))
        {
            diagnostics.info("artifact gc: no artifacts directory");
            return true;
        }
        std::vector<std::filesystem::path> inputs;
        if (!CacheReferences::localInputs(lockFile, workspaceRoot, artifactsRoot, inputs, diagnostics))
        {
            return false;
        }

        int deleteCount = 0;
        for (const auto &packageDir : std::filesystem::directory_iterator(artifactsRoot))
        {
            if (!packageDir.is_directory() || packageDir.is_symlink())
            {
                continue;
            }
            const auto packageName = packageDir.path().filename().string();
            if (!packageFilter.empty() && packageFilter != packageName)
            {
                continue;
            }
            for (const auto &artifactDir : std::filesystem::directory_iterator(packageDir.path()))
            {
                if (!artifactDir.is_directory() || artifactDir.is_symlink())
                {
                    continue;
                }
                const auto artifactId = packageName + "/" + artifactDir.path().filename().string();
                if (referenced.find(artifactId) != referenced.end() ||
                    CacheReferences::protectsInput(artifactDir.path(), inputs))
                {
                    continue;
                }
                ++deleteCount;
                diagnostics.info(std::string(apply ? "artifact gc delete " : "artifact gc dry-run ") +
                                 "artifact_id=" + artifactId + " package=" + packageName +
                                 " path=" + artifactDir.path().string() + " reason=not referenced by lockfile");
                if (apply)
                {
                    std::error_code ec;
                    std::filesystem::remove_all(artifactDir.path(), ec);
                    if (ec)
                    {
                        diagnostics.error("failed to delete artifact " + artifactId + ": " + ec.message());
                        return false;
                    }
                }
            }
        }
        if (deleteCount == 0)
        {
            diagnostics.info("artifact gc: nothing to delete");
        }
        return true;
    }

    bool ArtifactGc::runBuildCaches(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                    const BuildCacheGcOptions &request, DiagnosticSink &diagnostics)
    {
        const auto &[config, toolchainId, apply, packageFilter] = request;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return false;
        }

        std::map<std::string, std::set<std::string>> keepKeysByPackage;
        for (const auto &package : lockFile.packages)
        {
            keepKeysByPackage[package.name].insert(
                BuildConfig::normalize(package.config) + "-" +
                (package.toolchainId.empty() ? std::string("default") : package.toolchainId));
        }

        const auto buildRoot = Path::stateRoot(workspaceRoot) / "build-packages";
        if (!std::filesystem::is_directory(buildRoot) || std::filesystem::is_symlink(buildRoot))
        {
            diagnostics.info("build cache gc: no build-packages directory");
            return true;
        }
        std::vector<std::filesystem::path> inputs;
        if (!CacheReferences::localInputs(lockFile, workspaceRoot, buildRoot, inputs, diagnostics))
        {
            return false;
        }

        if (!packageFilter.empty() && keepKeysByPackage.find(packageFilter) != keepKeysByPackage.end())
        {
            keepKeysByPackage[packageFilter].insert(BuildConfig::normalize(config) + "-" +
                                                    (toolchainId.empty() ? std::string("default") : toolchainId));
        }
        int deleteCount = 0;
        for (const auto &packageDir : std::filesystem::directory_iterator(buildRoot))
        {
            if (!packageDir.is_directory() || packageDir.is_symlink())
            {
                continue;
            }
            const auto packageName = packageDir.path().filename().string();
            if (!packageFilter.empty() && packageFilter != packageName)
            {
                continue;
            }

            const auto keepIt = keepKeysByPackage.find(packageName);
            const bool packageDeclared = keepIt != keepKeysByPackage.end();
            for (const auto &cacheDir : std::filesystem::directory_iterator(packageDir.path()))
            {
                if (!cacheDir.is_directory() || cacheDir.is_symlink())
                {
                    continue;
                }
                const auto cacheName = cacheDir.path().filename().string();
                if ((packageDeclared && keepIt->second.find(cacheName) != keepIt->second.end()) ||
                    CacheReferences::protectsInput(cacheDir.path(), inputs))
                {
                    continue;
                }
                ++deleteCount;
                diagnostics.info(std::string(apply ? "build cache gc delete " : "build cache gc dry-run ") +
                                 "package=" + packageName + " cache=" + cacheName +
                                 " path=" + cacheDir.path().string() +
                                 (packageDeclared ? " reason=not current build workspace"
                                                  : " reason=package not referenced by lockfile"));
                if (apply)
                {
                    std::error_code ec;
                    std::filesystem::remove_all(cacheDir.path(), ec);
                    if (ec)
                    {
                        diagnostics.error("failed to delete build cache " + cacheDir.path().string() + ": " +
                                          ec.message());
                        return false;
                    }
                }
            }
        }
        if (deleteCount == 0)
        {
            diagnostics.info("build cache gc: nothing to delete");
        }
        return true;
    }

}
