#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/cli/Commands.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/ConsoleDiagnosticSink.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace rhodium
{
    int Commands::clean(const std::filesystem::path &workspaceRoot,
                     bool all,
                     bool artifacts,
                     bool sources,
                     bool lock,
                     const std::string &packageName)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        if (all && (artifacts || sources || !packageName.empty()))
        {
            diagnostics.error("clean --all already removes the active state directory; do not combine it with --artifacts, --sources, or a package name");
            return 1;
        }

        const auto rhodiumRoot = Path::stateRoot(workspaceRoot);
        std::error_code canonicalEc;
        auto normalizedWorkspaceRoot = std::filesystem::weakly_canonical(workspaceRoot, canonicalEc);
        if (canonicalEc)
        {
            canonicalEc.clear();
            normalizedWorkspaceRoot = std::filesystem::absolute(workspaceRoot, canonicalEc).lexically_normal();
            if (canonicalEc)
            {
                diagnostics.error("failed to resolve workspace root for clean: " + canonicalEc.message());
                return 1;
            }
        }
        const auto normalizedRHODIUMRoot = (normalizedWorkspaceRoot / rhodiumRoot.filename()).lexically_normal();
        auto isAllowedCleanPath = [&](const std::filesystem::path &path) -> bool
        {
            std::error_code ec;
            auto absolutePath = std::filesystem::absolute(path, ec);
            if (ec)
            {
                return false;
            }
            absolutePath = absolutePath.lexically_normal();

            const auto lowerPath = StringUtils::toLower(absolutePath.string());
            const auto lowerRHODIUMRoot = StringUtils::toLower(normalizedRHODIUMRoot.string());
            if (lowerPath == lowerRHODIUMRoot)
            {
                return true;
            }

            auto lowerPrefix = lowerRHODIUMRoot;
            if (!lowerPrefix.empty() && lowerPrefix.back() != '\\' && lowerPrefix.back() != '/')
            {
                lowerPrefix.push_back(std::filesystem::path::preferred_separator);
            }
            return lowerPath.rfind(lowerPrefix, 0) == 0;
        };

        auto removePath = [&](const std::filesystem::path &path) -> bool
        {
            if (!isAllowedCleanPath(path))
            {
                diagnostics.error("refusing to clean path outside the active state directory: " + path.string());
                return false;
            }
            std::error_code ec;
            if (!std::filesystem::exists(path, ec))
            {
                diagnostics.info("skip missing " + path.string());
                return true;
            }
            const auto removed = std::filesystem::remove_all(path, ec);
            if (ec)
            {
                diagnostics.error("failed to remove " + path.string() + ": " + ec.message());
                return false;
            }
            diagnostics.info("removed " + path.string() + " (" + std::to_string(removed) + " entries)");
            return true;
        };

        bool ok = true;
        if (all)
        {
            ok &= removePath(rhodiumRoot);
        }
        else
        {
            if (!packageName.empty())
            {
                ok &= removePath(rhodiumRoot / "build-packages" / Path::sanitizePackagePath(packageName));
                if (artifacts)
                {
                    ok &= removePath(rhodiumRoot / "artifacts" / Path::sanitizePackagePath(packageName));
                }
                if (sources)
                {
                    ok &= removePath(rhodiumRoot / "packages" / Path::sanitizePackagePath(packageName));
                }
            }
            else
            {
                ok &= removePath(rhodiumRoot / "generated");
                ok &= removePath(rhodiumRoot / "tmp");
                ok &= removePath(rhodiumRoot / "build-packages");
                if (artifacts)
                {
                    ok &= removePath(rhodiumRoot / "artifacts");
                }
                if (sources)
                {
                    ok &= removePath(rhodiumRoot / "packages");
                }
            }
        }
        if (lock)
        {
            std::error_code ec;
            const auto lockPath = workspaceRoot / "tpkg.lock.toml";
            if (!std::filesystem::exists(lockPath, ec))
            {
                diagnostics.info("skip missing " + lockPath.string());
            }
            else if (!std::filesystem::is_regular_file(lockPath, ec))
            {
                diagnostics.error("refusing to remove non-file lock path: " + lockPath.string());
                ok = false;
            }
            else
            {
                if (!packageName.empty())
                {
                    LockFile lockFile;
                    if (!LockFile::read(lockPath, lockFile, diagnostics))
                    {
                        ok = false;
                    }
                    else
                    {
                        const auto before = lockFile.packages.size();
                        lockFile.packages.erase(std::remove_if(lockFile.packages.begin(),
                                                               lockFile.packages.end(),
                                                               [&](const LockedPackage &package)
                                                               { return package.name == packageName; }),
                                                lockFile.packages.end());
                        if (lockFile.packages.size() == before)
                        {
                            diagnostics.info("skip missing lock entries for package " + packageName);
                        }
                        else if (!LockFile::write(lockPath, lockFile, diagnostics))
                        {
                            ok = false;
                        }
                        else
                        {
                            diagnostics.info("removed lock entries for package " + packageName);
                        }
                    }
                }
                else
                {
                    const auto removed = std::filesystem::remove(lockPath, ec);
                    if (ec)
                    {
                        diagnostics.error("failed to remove " + lockPath.string() + ": " + ec.message());
                        ok = false;
                    }
                    else if (!removed)
                    {
                        diagnostics.error("failed to remove " + lockPath.string());
                        ok = false;
                    }
                    else
                    {
                        diagnostics.info("removed " + lockPath.string());
                    }
                }
            }
        }
        return ok ? 0 : 1;
    }


} // namespace rhodium
