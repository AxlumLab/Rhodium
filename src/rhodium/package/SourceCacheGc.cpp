#include "rhodium/package/ArtifactGc.hpp"
#include "rhodium/package/CacheReferences.hpp"
#include "rhodium/package/GitFetcher.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/core/Hash.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/core/WorkspaceLock.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <exception>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <system_error>
#include <vector>

namespace rhodium
{
    bool ArtifactGc::runSourceCaches(const LockFile &lockFile,
                                     const std::filesystem::path &workspaceRoot,
                                     bool apply,
                                     const std::string &packageFilter,
                                     DiagnosticSink &diagnostics)
    {
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return false;
        }

        PackageCache cache(Path::stateRoot(workspaceRoot) / "packages");
        std::set<std::filesystem::path> keepGitSources;
        std::set<std::filesystem::path> keepArchiveRoots;
        std::map<std::filesystem::path, std::map<std::filesystem::path, std::set<std::filesystem::path>>> localSources;

        for (const auto &package : lockFile.packages)
        {
            if (package.name.empty() ||
                (package.sourceType != "git" && package.sourceType != "archive" && package.sourceType != "local") ||
                (package.sourceType != "local" && package.source.empty()))
            {
                diagnostics.error("cannot collect source caches with an invalid lock entry: " + package.name);
                return false;
            }
            if (package.sourceType == "git")
            {
                DependencyDesc dependency;
                dependency.name = package.name;
                dependency.source = package.source;
                dependency.ref = package.ref;
                keepGitSources.insert(cache.source(dependency).lexically_normal());
                try
                {
                    const auto local = GitFetcher::localPath(package.source, workspaceRoot);
                    if (!local.empty())
                    {
                        const auto name = std::filesystem::path(Path::sanitizePackagePath(package.name)).lexically_normal();
                        if (name.empty() || name == "." || name.has_root_path() || *name.begin() == "..")
                        {
                            diagnostics.error("invalid source cache package name: " + package.name);
                            return false;
                        }
                        const auto ref = package.ref.empty() ? std::string("head") : package.ref;
                        localSources[name][local].insert(std::filesystem::path(Path::sanitizePackagePath(ref)).lexically_normal());
                        const std::filesystem::path stored(package.source);
                        const auto absolute = stored.is_absolute() ? stored : workspaceRoot / stored;
                        for (const auto &spelling : {absolute.string(), absolute.generic_string(),
                                                    absolute.lexically_normal().string(), absolute.lexically_normal().generic_string()})
                        {
                            auto alias = dependency;
                            alias.source = spelling;
                            keepGitSources.insert(cache.source(alias).lexically_normal());
                        }
                    }
                }
                catch (const std::exception &ex)
                {
                    diagnostics.error("failed to resolve locked Git source: " + std::string(ex.what()));
                    return false;
                }
                continue;
            }
            if (package.sourceType == "archive")
            {
                const auto key = package.sha256.empty() ? Hash::xxhash64Hex(package.source) : package.sha256;
                if (!key.empty())
                {
                    keepArchiveRoots.insert((cache.root() / "_archives" / Path::sanitizePackagePath(package.name) / Path::sanitizePackagePath(key)).lexically_normal());
                }
                if (package.sha256.empty() && !package.resolvedArchiveHash.empty())
                {
                    keepArchiveRoots.insert((cache.root() / "_archives" / Path::sanitizePackagePath(package.name) / Path::sanitizePackagePath(package.resolvedArchiveHash)).lexically_normal());
                }
            }
        }

        const auto packagesRoot = Path::stateRoot(workspaceRoot) / "packages";
        if (!std::filesystem::is_directory(packagesRoot) || std::filesystem::is_symlink(packagesRoot))
        {
            diagnostics.info("source cache gc: no packages directory");
            return true;
        }
        std::vector<std::filesystem::path> inputs;
        if (!CacheReferences::localInputs(lockFile, workspaceRoot, packagesRoot, inputs, diagnostics))
        {
            return false;
        }
        for (const auto &input : inputs)
        {
            keepGitSources.insert(input);
            keepArchiveRoots.insert(input);
        }

        // Lockfiles store workspace-relative local sources; old source keys may
        // have been made from absolute spellings. Identify those repos by origin
        // rather than assuming the original spelling can be reconstructed.
        for (const auto &[name, sources] : localSources)
        {
            auto packageRoot = packagesRoot;
            bool symlink = false;
            for (const auto &component : name)
            {
                packageRoot /= component;
                if (std::filesystem::is_symlink(packageRoot))
                {
                    symlink = true;
                    break;
                }
            }
            if (symlink || !std::filesystem::is_directory(packageRoot))
            {
                continue;
            }
            std::error_code ec;
            std::filesystem::recursive_directory_iterator entries(packageRoot, ec);
            const std::filesystem::recursive_directory_iterator end;
            while (!ec && entries != end)
            {
                const auto path = entries->path();
                if (entries->is_symlink(ec))
                {
                    entries.disable_recursion_pending();
                }
                else if (entries->is_directory(ec) && std::filesystem::is_directory(path / ".git"))
                {
                    entries.disable_recursion_pending();
                    if (keepGitSources.find(path.lexically_normal()) != keepGitSources.end())
                    {
                        entries.increment(ec);
                        continue;
                    }
                    std::string origin;
                    bool keep = !GitFetcher::cachedOrigin(path, origin);
                    if (keep)
                    {
                        diagnostics.warning("cannot inspect cached Git origin; preserving " + path.string());
                    }
                    try
                    {
                        const auto found = sources.find(GitFetcher::localPath(origin, workspaceRoot));
                        const auto relative = path.lexically_relative(packageRoot);
                        auto component = relative.begin();
                        if (component != relative.end())
                        {
                            ++component; // The first component is the original source hash.
                        }
                        std::filesystem::path ref;
                        for (; component != relative.end(); ++component)
                        {
                            ref /= *component;
                        }
                        keep = keep || (found != sources.end() && found->second.find(ref.lexically_normal()) != found->second.end());
                    }
                    catch (const std::exception &)
                    {
                        keep = true;
                        diagnostics.warning("cannot resolve cached Git origin; preserving " + path.string());
                    }
                    if (keep)
                    {
                        keepGitSources.insert(path.lexically_normal());
                    }
                }
                if (!ec)
                {
                    entries.increment(ec);
                }
            }
            if (ec)
            {
                diagnostics.error("failed to inspect local Git source caches: " + ec.message());
                return false;
            }
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

        auto pruneSource = [&](auto &&self, const std::filesystem::path &path,
                               const std::set<std::filesystem::path> &keptPaths,
                               const std::string &packageName) -> bool
        {
            std::error_code ec;
            if (std::filesystem::is_symlink(path, ec))
            {
                diagnostics.warning("skipping symlink in source cache: " + path.string());
                return true;
            }
            if (ec)
            {
                diagnostics.error("failed to inspect source cache: " + ec.message());
                return false;
            }
            const auto normalized = path.lexically_normal();
            if (keptPaths.find(normalized) != keptPaths.end())
            {
                return true;
            }
            bool containsKeptSource = false;
            for (const auto &kept : keptPaths)
            {
                if (CacheReferences::within(normalized, kept))
                {
                    return true;
                }
                const auto relative = kept.lexically_relative(normalized);
                if (!relative.empty() && !relative.is_absolute() && *relative.begin() != "..")
                {
                    containsKeptSource = true;
                    break;
                }
            }
            if (!containsKeptSource)
            {
                return removeCacheDir(path, "package=" + packageName + " reason=not referenced by lockfile");
            }
            std::filesystem::directory_iterator entries(path, ec);
            const std::filesystem::directory_iterator end;
            while (!ec && entries != end)
            {
                const auto entry = *entries;
                entries.increment(ec);
                if (ec)
                {
                    break;
                }
                if (entry.is_directory(ec) && !self(self, entry.path(), keptPaths, packageName))
                {
                    return false;
                }
            }
            if (ec)
            {
                diagnostics.error("failed to scan source cache: " + ec.message());
                return false;
            }
            return true;
        };

        auto sweep = [&](const std::filesystem::path &root,
                         const std::set<std::filesystem::path> &keptPaths, bool git) -> bool
        {
            if (!std::filesystem::is_directory(root) || std::filesystem::is_symlink(root))
            {
                return true;
            }
            if (!packageFilter.empty())
            {
                const auto relative = std::filesystem::path(Path::sanitizePackagePath(packageFilter)).lexically_normal();
                if (relative.empty() || relative == "." || relative.has_root_path() || *relative.begin() == "..")
                {
                    diagnostics.error("invalid package source cache filter: " + packageFilter);
                    return false;
                }
                if (git && *relative.begin() == "_archives")
                {
                    return true;
                }
                auto target = root;
                for (const auto &component : relative)
                {
                    target /= component;
                    std::error_code ec;
                    if (std::filesystem::is_symlink(target, ec))
                    {
                        return true;
                    }
                    if (ec && ec != std::errc::no_such_file_or_directory)
                    {
                        diagnostics.error("failed to inspect filtered source cache: " + ec.message());
                        return false;
                    }
                }
                return !std::filesystem::is_directory(target) || pruneSource(pruneSource, target, keptPaths, packageFilter);
            }
            std::error_code ec;
            std::filesystem::directory_iterator entries(root, ec);
            const std::filesystem::directory_iterator end;
            while (!ec && entries != end)
            {
                const auto entry = *entries;
                entries.increment(ec);
                if (ec)
                {
                    break;
                }
                if (git && entry.path().filename() == "_archives")
                {
                    continue;
                }
                if (entry.is_directory(ec) &&
                    !pruneSource(pruneSource, entry.path(), keptPaths, entry.path().filename().string()))
                {
                    return false;
                }
            }
            if (ec)
            {
                diagnostics.error("failed to scan source cache: " + ec.message());
                return false;
            }
            return true;
        };
        if (!sweep(packagesRoot, keepGitSources, true) ||
            !sweep(packagesRoot / "_archives", keepArchiveRoots, false))
        {
            return false;
        }

        if (deleteCount == 0)
        {
            diagnostics.info("source cache gc: nothing to delete");
        }
        return true;
    }
}
