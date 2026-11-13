/*
 * @Author: khfahqp khfahqp@gmail.com
 * @LastEditors: khfahqp khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "CleanSession.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/core/StringUtil.hpp"

#include <algorithm>

namespace rhodium
{
    CleanSession::CleanSession(const std::filesystem::path &m_workspaceRoot, const CleanOptions &options,
                               DiagnosticSink &m_diagnostics)

        : m_workspaceRoot(m_workspaceRoot)
        , m_options(options)
        , m_diagnostics(m_diagnostics)
        , m_rhodiumRoot(Path::stateRoot(m_workspaceRoot))
    {
    }

    int CleanSession::run()
    {
        const auto &[all, artifacts, sources, lock, packageName] = m_options;
        if (all && (artifacts || sources || !packageName.empty()))
        {
            m_diagnostics.error(
                "clean --all already removes the active state directory; do not combine it with "
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

    bool CleanSession::resolveRoot()
    {
        std::error_code canonicalEc;
        auto normalizedWorkspaceRoot = std::filesystem::weakly_canonical(m_workspaceRoot, canonicalEc);
        if (canonicalEc)
        {
            canonicalEc.clear();
            normalizedWorkspaceRoot = std::filesystem::absolute(m_workspaceRoot, canonicalEc).lexically_normal();
            if (canonicalEc)
            {
                m_diagnostics.error("failed to resolve workspace root for clean: " + canonicalEc.message());
                return false;
            }
        }
        m_normalizedRHODIUMRoot = (normalizedWorkspaceRoot / m_rhodiumRoot.filename()).lexically_normal();

        return true;
    }

    bool CleanSession::allowed(const std::filesystem::path &path) const
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
        const auto lowerRHODIUMRoot = StringUtils::toLower(m_normalizedRHODIUMRoot.string());
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

    bool CleanSession::remove(const std::filesystem::path &path)
    {
        if (!allowed(path))
        {
            m_diagnostics.error("refusing to clean path outside the active state directory: " + path.string());
            return false;
        }
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            m_diagnostics.info("skip missing " + path.string());
            return true;
        }
        auto removalPath = std::filesystem::absolute(path, ec).lexically_normal();
        if (ec)
        {
            m_diagnostics.error("failed to resolve clean path: " + ec.message());
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
            m_diagnostics.error("failed to remove " + path.string() + ": " + ec.message());
            return false;
        }
        m_diagnostics.info("removed " + path.string() + " (" + std::to_string(removed) + " entries)");
        return true;
    }

    bool CleanSession::cleanCaches()
    {
        const auto &[all, artifacts, sources, lock, packageName] = m_options;
        bool ok = true;
        if (all)
        {
            ok &= remove(m_rhodiumRoot);
        }
        else
        {
            if (!packageName.empty())
            {
                ok &= remove(m_rhodiumRoot / "build-packages" / Path::sanitizePackagePath(packageName));
                if (artifacts)
                {
                    ok &= remove(m_rhodiumRoot / "artifacts" / Path::sanitizePackagePath(packageName));
                }
                if (sources)
                {
                    ok &= remove(m_rhodiumRoot / "packages" / Path::sanitizePackagePath(packageName));
                }
            }
            else
            {
                ok &= remove(m_rhodiumRoot / "generated");
                ok &= remove(m_rhodiumRoot / "tmp");
                ok &= remove(m_rhodiumRoot / "build-packages");
                if (artifacts)
                {
                    ok &= remove(m_rhodiumRoot / "artifacts");
                }
                if (sources)
                {
                    ok &= remove(m_rhodiumRoot / "packages");
                }
            }
        }

        return ok;
    }

    bool CleanSession::cleanLock()
    {
        const auto &packageName = m_options.packageName;
        bool ok = true;

        std::error_code ec;
        const auto lockPath = Path::lockFile(m_workspaceRoot);
        if (!std::filesystem::exists(lockPath, ec))
        {
            m_diagnostics.info("skip missing " + lockPath.string());
        }
        else if (!std::filesystem::is_regular_file(lockPath, ec))
        {
            m_diagnostics.error("refusing to remove non-file lock path: " + lockPath.string());
            ok = false;
        }
        else
        {
            if (!packageName.empty())
            {
                LockFile lockFile;
                if (!LockFile::read(lockPath, lockFile, m_diagnostics))
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
                        m_diagnostics.info("skip missing lock entries for package " + packageName);
                    }
                    else if (!LockFile::write(lockPath, lockFile, m_diagnostics))
                    {
                        ok = false;
                    }
                    else
                    {
                        m_diagnostics.info("removed lock entries for package " + packageName);
                    }
                }
            }
            else
            {
                const auto removed = std::filesystem::remove(lockPath, ec);
                if (ec)
                {
                    m_diagnostics.error("failed to remove " + lockPath.string() + ": " + ec.message());
                    ok = false;
                }
                else if (!removed)
                {
                    m_diagnostics.error("failed to remove " + lockPath.string());
                    ok = false;
                }
                else
                {
                    m_diagnostics.info("removed " + lockPath.string());
                }
            }
        }

        return ok;
    }

} // namespace rhodium