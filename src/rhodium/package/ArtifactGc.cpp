/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/ArtifactGc.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/core/Hash.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageCache.hpp"

#include <filesystem>
#include <map>
#include <set>

namespace rhodium
{
    bool ArtifactGc::runArtifacts(const LockFile &lockFile,
                                  const std::filesystem::path &workspaceRoot,
                                  bool apply,
                                  const std::string &packageFilter,
                                  DiagnosticSink &diagnostics)
    {
        std::set<std::string> referenced;
        for (const auto &package : lockFile.packages)
        {
            if (!package.artifactId.empty())
            {
                referenced.insert(package.artifactId);
            }
        }

        const auto artifactsRoot = Path::stateRoot(workspaceRoot) / "artifacts";
        if (!std::filesystem::is_directory(artifactsRoot))
        {
            diagnostics.info("artifact gc: no artifacts directory");
            return true;
        }

        int deleteCount = 0;
        for (const auto &packageDir : std::filesystem::directory_iterator(artifactsRoot))
        {
            if (!packageDir.is_directory())
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
                if (!artifactDir.is_directory())
                {
                    continue;
                }
                const auto artifactId = packageName + "/" + artifactDir.path().filename().string();
                if (referenced.find(artifactId) != referenced.end())
                {
                    continue;
                }
                ++deleteCount;
                diagnostics.info(std::string(apply ? "artifact gc delete " : "artifact gc dry-run ") +
                                 "artifact_id=" + artifactId +
                                 " package=" + packageName +
                                 " path=" + artifactDir.path().string() +
                                 " reason=not referenced by lockfile");
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

    bool ArtifactGc::runBuildCaches(const LockFile &lockFile,
                                    const std::filesystem::path &workspaceRoot,
                                    const std::string &config,
                                    const std::string &toolchainId,
                                    bool apply,
                                    const std::string &packageFilter,
                                    DiagnosticSink &diagnostics)
    {
        std::map<std::string, std::set<std::string>> keepKeysByPackage;
        for (const auto &package : lockFile.packages)
        {
            keepKeysByPackage[package.name].insert(BuildConfig::normalize(package.config) + "-" + (package.toolchainId.empty() ? std::string("default") : package.toolchainId));
        }

        const auto buildRoot = Path::stateRoot(workspaceRoot) / "build-packages";
        if (!std::filesystem::is_directory(buildRoot))
        {
            diagnostics.info("build cache gc: no build-packages directory");
            return true;
        }

        if (!packageFilter.empty() && keepKeysByPackage.find(packageFilter) != keepKeysByPackage.end())
        {
            keepKeysByPackage[packageFilter].insert(BuildConfig::normalize(config) + "-" + (toolchainId.empty() ? std::string("default") : toolchainId));
        }
        int deleteCount = 0;
        for (const auto &packageDir : std::filesystem::directory_iterator(buildRoot))
        {
            if (!packageDir.is_directory())
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
                if (!cacheDir.is_directory())
                {
                    continue;
                }
                const auto cacheName = cacheDir.path().filename().string();
                if (packageDeclared && keepIt->second.find(cacheName) != keepIt->second.end())
                {
                    continue;
                }
                ++deleteCount;
                diagnostics.info(std::string(apply ? "build cache gc delete " : "build cache gc dry-run ") +
                                 "package=" + packageName +
                                 " cache=" + cacheName +
                                 " path=" + cacheDir.path().string() +
                                 (packageDeclared ? " reason=not current build workspace" : " reason=package not referenced by lockfile"));
                if (apply)
                {
                    std::error_code ec;
                    std::filesystem::remove_all(cacheDir.path(), ec);
                    if (ec)
                    {
                        diagnostics.error("failed to delete build cache " + cacheDir.path().string() + ": " + ec.message());
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

    bool ArtifactGc::runSourceCaches(const LockFile &lockFile,
                                     const std::filesystem::path &workspaceRoot,
                                     bool apply,
                                     const std::string &packageFilter,
                                     DiagnosticSink &diagnostics)
    {
        PackageCache cache(Path::stateRoot(workspaceRoot) / "packages");
        std::set<std::filesystem::path> keepGitSources;
        std::set<std::filesystem::path> keepArchiveRoots;
        std::set<std::string> keptArchivePackages;

        for (const auto &package : lockFile.packages)
        {
            if (!packageFilter.empty() && package.name != packageFilter)
            {
                continue;
            }
            if (package.sourceType == "git")
            {
                DependencyDesc dependency;
                dependency.name = package.name;
                dependency.source = package.source;
                dependency.ref = package.ref;
                keepGitSources.insert(cache.source(dependency).lexically_normal());
                continue;
            }
            if (package.sourceType == "archive")
            {
                const auto key = package.sha256.empty() ? Hash::xxhash64Hex(package.source) : package.sha256;
                if (!key.empty())
                {
                    keepArchiveRoots.insert((cache.root() / "_archives" / Path::sanitizePackagePath(package.name) / Path::sanitizePackagePath(key)).lexically_normal());
                    keptArchivePackages.insert(package.name);
                }
                if (package.sha256.empty() && !package.resolvedArchiveHash.empty())
                {
                    keepArchiveRoots.insert((cache.root() / "_archives" / Path::sanitizePackagePath(package.name) / Path::sanitizePackagePath(package.resolvedArchiveHash)).lexically_normal());
                    keptArchivePackages.insert(package.name);
                }
            }
        }

        const auto packagesRoot = Path::stateRoot(workspaceRoot) / "packages";
        if (!std::filesystem::is_directory(packagesRoot))
        {
            diagnostics.info("source cache gc: no packages directory");
            return true;
        }

        int deleteCount = 0;
        auto removeCacheDir = [&](const std::filesystem::path &path, const std::string &description) -> bool
        {
            ++deleteCount;
            diagnostics.info(std::string(apply ? "source cache gc delete " : "source cache gc dry-run ") +
                             description +
                             " path=" + path.string());
            if (!apply)
            {
                return true;
            }
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
            if (ec)
            {
                diagnostics.error("failed to delete source cache " + path.string() + ": " + ec.message());
                return false;
            }
            return true;
        };

        auto pruneGitSource = [&](auto &&self, const std::filesystem::path &path, const std::string &packageName) -> bool
        {
            std::error_code inspectError;
            if (std::filesystem::is_symlink(path, inspectError))
            {
                diagnostics.warning("skipping symlink in git source cache: " + path.string());
                return true;
            }
            if (inspectError)
            {
                diagnostics.error("failed to inspect git ref directory: " + inspectError.message());
                return false;
            }
            const auto normalized = path.lexically_normal();
            if (keepGitSources.find(normalized) != keepGitSources.end())
            {
                return true;
            }
            bool containsKeptSource = false;
            for (const auto &kept : keepGitSources)
            {
                const auto relative = kept.lexically_relative(normalized);
                if (!relative.empty() && !relative.is_absolute() && *relative.begin() != "..")
                {
                    containsKeptSource = true;
                    break;
                }
            }
            if (!containsKeptSource)
            {
                return removeCacheDir(path, "package=" + packageName + " reason=not current git ref");
            }

            // Refs containing '/' share parent directories; prune only their unused children.
            std::error_code ec;
            std::filesystem::directory_iterator entries(path, ec);
            if (ec)
            {
                diagnostics.error("failed to scan git ref directory: " + ec.message());
                return false;
            }
            const std::filesystem::directory_iterator end;
            while (entries != end)
            {
                const auto entry = *entries;
                entries.increment(ec);
                if (ec)
                {
                    diagnostics.error("failed to scan git ref directory: " + ec.message());
                    return false;
                }
                if (entry.is_directory(ec) && !entry.is_symlink(ec) && !self(self, entry.path(), packageName))
                {
                    return false;
                }
                if (ec)
                {
                    diagnostics.error("failed to inspect git ref directory: " + ec.message());
                    return false;
                }
            }
            return true;
        };

        for (const auto &package : lockFile.packages)
        {
            if (package.sourceType != "git" || (!packageFilter.empty() && package.name != packageFilter))
            {
                continue;
            }
            DependencyDesc dependency;
            dependency.name = package.name;
            dependency.source = package.source;
            dependency.ref = package.ref;
            const auto sourceRoot = cache.sourceRoot(dependency);
            std::error_code ec;
            if (!std::filesystem::is_directory(sourceRoot, ec))
            {
                continue;
            }
            for (const auto &entry : std::filesystem::directory_iterator(sourceRoot, ec))
            {
                if (ec)
                {
                    diagnostics.warning("failed to scan package source cache: " + ec.message());
                    return false;
                }
                if (!entry.is_directory(ec))
                {
                    continue;
                }
                if (!pruneGitSource(pruneGitSource, entry.path(), package.name))
                {
                    return false;
                }
            }
        }

        const auto archivesRoot = packagesRoot / "_archives";
        if (std::filesystem::is_directory(archivesRoot))
        {
            std::error_code ec;
            for (const auto &packageDir : std::filesystem::directory_iterator(archivesRoot, ec))
            {
                if (ec)
                {
                    diagnostics.warning("failed to scan archive source cache: " + ec.message());
                    return false;
                }
                if (!packageDir.is_directory(ec))
                {
                    continue;
                }
                const auto packageName = packageDir.path().filename().string();
                if (!packageFilter.empty() && packageFilter != packageName)
                {
                    continue;
                }
                for (const auto &cacheDir : std::filesystem::directory_iterator(packageDir.path(), ec))
                {
                    if (ec)
                    {
                        diagnostics.warning("failed to scan archive package cache: " + ec.message());
                        return false;
                    }
                    if (!cacheDir.is_directory(ec))
                    {
                        continue;
                    }
                    const auto path = cacheDir.path().lexically_normal();
                    if (keepArchiveRoots.find(path) != keepArchiveRoots.end())
                    {
                        continue;
                    }
                    const auto reason = keptArchivePackages.find(packageName) == keptArchivePackages.end()
                                            ? " reason=archive package not referenced by lockfile"
                                            : " reason=not current archive cache";
                    if (!removeCacheDir(cacheDir.path(), "package=" + packageName + reason))
                    {
                        return false;
                    }
                }
            }
        }

        if (deleteCount == 0)
        {
            diagnostics.info("source cache gc: nothing to delete");
        }
        return true;
    }
}
