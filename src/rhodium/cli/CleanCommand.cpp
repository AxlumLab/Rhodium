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
    namespace
    {
        class CleanSession final
        {
        public:
            CleanSession(const std::filesystem::path &workspaceRoot, const CleanOptions &options,
                         DiagnosticSink &diagnostics)
                : workspaceRoot(workspaceRoot), options(options), diagnostics(diagnostics),
                  rhodiumRoot(Path::stateRoot(workspaceRoot))
            {
            }

            int run()
            {
                const auto &[all, artifacts, sources, lock, packageName] = options;
                if (all && (artifacts || sources || !packageName.empty()))
                {
                    diagnostics.error("clean --all already removes the active state directory; do not combine it with "
                                      "--artifacts, --sources, or a package name");
                    return 1;
                }
                if (!resolveRoot())
                {
                    return 1;
                }
                bool ok = cleanCaches();
                if (lock)
                {
                    ok &= cleanLock();
                }
                return ok ? 0 : 1;
            }

        private:
            bool resolveRoot()
            {
                std::error_code canonicalEc;
                auto normalizedWorkspaceRoot = std::filesystem::weakly_canonical(workspaceRoot, canonicalEc);
                if (canonicalEc)
                {
                    canonicalEc.clear();
                    normalizedWorkspaceRoot = std::filesystem::absolute(workspaceRoot, canonicalEc).lexically_normal();
                    if (canonicalEc)
                    {
                        diagnostics.error("failed to resolve workspace root for clean: " + canonicalEc.message());
                        return false;
                    }
                }
                normalizedRHODIUMRoot = (normalizedWorkspaceRoot / rhodiumRoot.filename()).lexically_normal();

                return true;
            }

            bool allowed(const std::filesystem::path &path) const
            {
                std::error_code ec;
                auto absolutePath = std::filesystem::absolute(path, ec);
                if (ec)
                {
                    return false;
                }
                absolutePath = absolutePath.lexically_normal();
                // Resolve ancestor aliases (including Windows short names), but do not
                // follow the final entry: cleaning a symlink must remove the link itself.
                const auto parent = std::filesystem::weakly_canonical(absolutePath.parent_path(), ec);
                if (ec)
                {
                    return false;
                }
                absolutePath = (parent / absolutePath.filename()).lexically_normal();

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
            }

            bool remove(const std::filesystem::path &path)
            {
                if (!allowed(path))
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
                auto removalPath = std::filesystem::absolute(path, ec).lexically_normal();
                if (ec)
                {
                    diagnostics.error("failed to resolve clean path: " + ec.message());
                    return false;
                }
#ifdef _WIN32
                // The root can be short while descendants exceed MAX_PATH.
                auto nativePath = removalPath.make_preferred().native();
                if (nativePath.rfind(L"\\\\?\\", 0) != 0)
                {
                    nativePath = nativePath.rfind(L"\\\\", 0) == 0 ? L"\\\\?\\UNC\\" + nativePath.substr(2)
                                                                   : L"\\\\?\\" + nativePath;
                    removalPath = nativePath;
                }
#endif
                const auto removed = std::filesystem::remove_all(removalPath, ec);
                if (ec)
                {
                    diagnostics.error("failed to remove " + path.string() + ": " + ec.message());
                    return false;
                }
                diagnostics.info("removed " + path.string() + " (" + std::to_string(removed) + " entries)");
                return true;
            }

            bool cleanCaches()
            {
                const auto &[all, artifacts, sources, lock, packageName] = options;
                bool ok = true;
                if (all)
                {
                    ok &= remove(rhodiumRoot);
                }
                else
                {
                    if (!packageName.empty())
                    {
                        ok &= remove(rhodiumRoot / "build-packages" / Path::sanitizePackagePath(packageName));
                        if (artifacts)
                        {
                            ok &= remove(rhodiumRoot / "artifacts" / Path::sanitizePackagePath(packageName));
                        }
                        if (sources)
                        {
                            ok &= remove(rhodiumRoot / "packages" / Path::sanitizePackagePath(packageName));
                        }
                    }
                    else
                    {
                        ok &= remove(rhodiumRoot / "generated");
                        ok &= remove(rhodiumRoot / "tmp");
                        ok &= remove(rhodiumRoot / "build-packages");
                        if (artifacts)
                        {
                            ok &= remove(rhodiumRoot / "artifacts");
                        }
                        if (sources)
                        {
                            ok &= remove(rhodiumRoot / "packages");
                        }
                    }
                }

                return ok;
            }

            bool cleanLock()
            {
                const auto &packageName = options.packageName;
                bool ok = true;

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
                            lockFile.packages.erase(std::remove_if(lockFile.packages.begin(), lockFile.packages.end(),
                                                                   [&](const LockedPackage &package)
                                                                   {
                                                                       return package.name == packageName;
                                                                   }),
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

                return ok;
            }

            const std::filesystem::path &workspaceRoot;
            const CleanOptions &options;
            DiagnosticSink &diagnostics;
            const std::filesystem::path rhodiumRoot;
            std::filesystem::path normalizedRHODIUMRoot;
        };
    }

    int Commands::clean(const std::filesystem::path &workspaceRoot, const CleanOptions &options)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        return CleanSession(workspaceRoot, options, diagnostics).run();
    }

} // namespace rhodium
