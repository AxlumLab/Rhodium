/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/Path.hpp"

#include "rhodium/resolve/DependencyFetchStep.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/Archive.hpp"
#include "rhodium/package/GitFetcher.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/resolve/DependencyExportStep.hpp"
#include "rhodium/resolve/DependencyLockStep.hpp"

#include <system_error>

namespace rhodium
{

    bool DepFetch::sourceLocal(const Request &request, const Context &context, Result &result,
                               DiagnosticSink &diagnostics)
    {
        const auto &[dependency, locked, wantsUpdate] = request;

        result.sourceRoot = local(context.workspaceRoot, dependency);
        if (!File::dir(result.sourceRoot))
        {
            diagnostics.error(std::string(dependency.overridden ? "dependency override path is missing: "
                                                                : "local dependency source is missing: ") +
                              result.sourceRoot.string());
            return false;
        }
        return true;
    }

    bool DepFetch::sourceArchive(const Request &request, const Context &context, Result &result,
                                 DiagnosticSink &diagnostics)
    {
        const auto &[dependency, locked, wantsUpdate] = request;

        ArchiveFetcher archive;
        ArchiveFetchResult archiveResult;
        if (!archive.fetchAndExtract(dependency, context.workspaceRoot, context.cache.root(), archiveResult,
                                     diagnostics))
        {
            return false;
        }
        dependency.resolvedArchiveHash = archiveResult.archiveHash;
        dependency.patchHashes = archiveResult.patchHashes;
        result.commit = archiveResult.archiveHash;
        result.sourceRoot = archiveResult.sourceDir;

        if (locked && !wantsUpdate && !context.options.rebuild && !context.options.buildOnly &&
            !context.options.exportOnly)
        {
            LockedPackage expected =
                LockedPackage::from(dependency, result.commit, context.effectiveConfig, context.preferredToolchain,
                                    context.options.toolchainContentHash);
            expected.source = DepLock::source(context.workspaceRoot, dependency);
            if (DepExport::reusable(*locked, expected,
                                    Path::stateRoot(context.workspaceRoot) / "artifacts" / expected.artifactId))
            {
                diagnostics.info("package artifact is up to date; skipping archive build: " + dependency.name);
                result.skipPackage = true;
            }
        }
        return true;
    }

    bool DepFetch::sourceGit(const Request &request, const Context &context, Result &result,
                             DiagnosticSink &diagnostics)
    {
        const auto &[dependency, locked, wantsUpdate] = request;

        if (context.options.exportOnly)
        {
            if (!locked || result.commit.empty())
            {
                diagnostics.error("restore --export-only requires an existing lock entry for git package " +
                                  dependency.name);
                return false;
            }
            result.sourceRoot = context.cache.source(dependency);
            return true;
        }

        if (locked && !result.commit.empty() && !wantsUpdate && !context.options.rebuild && !context.options.buildOnly)
        {
            LockedPackage expected =
                LockedPackage::from(dependency, result.commit, context.effectiveConfig, context.preferredToolchain,
                                    context.options.toolchainContentHash);
            expected.source = DepLock::source(context.workspaceRoot, dependency);
            if (DepExport::reusable(*locked, expected,
                                    Path::stateRoot(context.workspaceRoot) / "artifacts" / expected.artifactId))
            {
                diagnostics.info("package artifact is up to date; skipping git sync and build: " + dependency.name);
                result.skipPackage = true;
                return true;
            }
        }

        const auto stableSourcePath = context.cache.source(dependency);
        auto initialPath = stableSourcePath;
        if (!File::dir(initialPath / ".git"))
        {
            const auto reusablePath = reusable(context, dependency, initialPath);
            if (!reusablePath.empty())
            {
                initialPath = reusablePath;
                move(initialPath, stableSourcePath, diagnostics);
                initialPath = File::dir(stableSourcePath / ".git") ? stableSourcePath : initialPath;
                diagnostics.info("reusing package source cache: " + dependency.name + " from " + initialPath.string());
            }
        }
        auto fetchDependency = forGit(context.workspaceRoot, dependency);
        if (locked && !result.commit.empty())
        {
            fetchDependency.ref = result.commit;
        }
        if (!context.git.fetch(fetchDependency, initialPath, result.commit, diagnostics, wantsUpdate))
        {
            return false;
        }
        result.sourceRoot = initialPath;
        return true;
    }

    bool DepFetch::source(const Request &request, const Context &context, Result &result, DiagnosticSink &diagnostics)
    {
        const auto &[dependency, locked, wantsUpdate] = request;
        result = {};
        result.commit =
            dependency.overridden
                ? "override"
                : (dependency.sourceType == "local" ? "local" : ((locked && !wantsUpdate) ? locked->commit : ""));
        if (dependency.sourceType == "local")
            return sourceLocal(request, context, result, diagnostics);
        if (dependency.sourceType == "archive")
            return sourceArchive(request, context, result, diagnostics);
        if (dependency.sourceType == "git")
            return sourceGit(request, context, result, diagnostics);
        diagnostics.error("unsupported dependency source type: " + dependency.sourceType + " for " + dependency.name);
        return false;
    }

    std::filesystem::path DepFetch::local(const std::filesystem::path &workspaceRoot, const DependencyDesc &dependency)
    {
        auto path = std::filesystem::path(dependency.source);
        if (path.is_relative())
        {
            path = workspaceRoot / path;
        }
        return path;
    }

    std::filesystem::path DepFetch::reusable(const Context &context, const DependencyDesc &dependency,
                                             const std::filesystem::path &preferredPath)
    {
        const auto &cache = context.cache;
        if (File::dir(preferredPath / ".git"))
        {
            return preferredPath;
        }

        const auto sourceRoot = cache.sourceRoot(dependency);
        std::error_code ec;
        if (!std::filesystem::is_directory(sourceRoot, ec))
        {
            return {};
        }

        std::filesystem::path candidate;
        int candidates = 0;
        for (const auto &entry : std::filesystem::directory_iterator(sourceRoot, ec))
        {
            if (ec)
            {
                return {};
            }
            if (!entry.is_directory(ec) || entry.is_symlink(ec))
            {
                continue;
            }
            const auto path = entry.path();
            if (!File::dir(path / ".git"))
            {
                continue;
            }
            bool referenced = false;
            const auto ref = path.lexically_relative(sourceRoot).generic_string();
            for (const auto &lockedEntry : context.lockFile.packages)
            {
                const auto entryRef = lockedEntry.ref.empty() ? std::string("head") : lockedEntry.ref;
                if (lockedEntry.name == dependency.name && lockedEntry.sourceType == "git" &&
                    Path::sanitizePackagePath(entryRef) == ref)
                {
                    referenced = true;
                    break;
                }
            }
            if (referenced)
            {
                continue;
            }
            candidate = path;
            ++candidates;
            if (candidates > 1)
            {
                return {};
            }
        }
        return candidates == 1 ? candidate : std::filesystem::path{};
    }

    void DepFetch::move(const std::filesystem::path &from, const std::filesystem::path &to, DiagnosticSink &diagnostics)
    {
        if (from.empty() || from == to || File::dir(to))
        {
            return;
        }

        std::string error;
        if (!File::mkdir(to.parent_path(), &error))
        {
            diagnostics.warning("failed to create package source parent: " + error);
            return;
        }

        std::error_code ec;
        std::filesystem::rename(from, to, ec);
        if (ec)
        {
            diagnostics.warning("failed to move package source cache to stable ref path: " + ec.message());
            return;
        }
        diagnostics.info("moved package source cache to " + to.string());
    }

    bool DepFetch::urlLike(const std::string &source)
    {
        return source.rfind("http://", 0) == 0 || source.rfind("https://", 0) == 0 || source.rfind("file://", 0) == 0 ||
               source.rfind("ssh://", 0) == 0 || source.rfind("git@", 0) == 0 || source.rfind("github.com/", 0) == 0;
    }

    DependencyDesc DepFetch::forGit(const std::filesystem::path &workspaceRoot, const DependencyDesc &dependency)
    {
        auto fetchDependency = dependency;
        if (!urlLike(fetchDependency.source))
        {
            auto path = std::filesystem::path(fetchDependency.source);
            if (path.is_relative() && File::dir(workspaceRoot / path))
            {
                fetchDependency.source = (workspaceRoot / path).string();
            }
        }
        return fetchDependency;
    }

} // namespace rhodium
