#include "rhodium/resolve/DependencyLockStep.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/resolve/DependencyFetchStep.hpp"

#include <algorithm>

namespace rhodium
{

    bool DepLock::same(const LockedPackage &package, const DependencyDesc &dependency)
    {
        if (package.name != dependency.name || package.overridden != dependency.overridden)
        {
            return false;
        }
        if (dependency.overridden)
        {
            return package.overridePath == dependency.overridePath;
        }
        if (package.source != dependency.source)
        {
            return false;
        }
        if (package.sourceType != dependency.sourceType)
        {
            return false;
        }
        if (package.ref != dependency.ref)
        {
            return false;
        }
        if (package.sourceType == "archive")
        {
            if (!dependency.sha256.empty() && package.sha256 != dependency.sha256)
            {
                return false;
            }
            if (!dependency.resolvedArchiveHash.empty() && package.resolvedArchiveHash != dependency.resolvedArchiveHash)
            {
                return false;
            }
        }
        return true;
    }

    bool DepLock::matches(const LockedPackage &package,
                          const DependencyDesc &dependency,
                          const std::string &config,
                          const std::string &toolchainId)
    {
        if (!same(package, dependency))
        {
            return false;
        }
        return BuildConfig::normalize(package.config) == BuildConfig::normalize(config) &&
               package.toolchainId == toolchainId;
    }

    bool DepLock::legacy(const LockedPackage &package,
                         const DependencyDesc &dependency,
                         const std::string &config)
    {
        if (!same(package, dependency))
        {
            return false;
        }
        return BuildConfig::normalize(config) == "debug" &&
               BuildConfig::normalize(package.config) == "debug" &&
               package.toolchainId.empty();
    }

    LockedPackage *DepLock::find(LockFile &lockFile,
                                 const DependencyDesc &dependency,
                                 const std::string &config,
                                 const std::string &toolchainId)
    {
        auto existing = std::find_if(lockFile.packages.begin(), lockFile.packages.end(), [&](const LockedPackage &package)
                                     { return matches(package, dependency, config, toolchainId); });
        if (existing != lockFile.packages.end())
        {
            return &*existing;
        }
        auto legacyMatch = std::find_if(lockFile.packages.begin(), lockFile.packages.end(), [&](const LockedPackage &package)
                                        { return legacy(package, dependency, config); });
        return legacyMatch == lockFile.packages.end() ? nullptr : &*legacyMatch;
    }

    std::string DepLock::source(const std::filesystem::path &workspaceRoot, const DependencyDesc &dependency)
    {
        if (DepFetch::urlLike(dependency.source))
        {
            return dependency.source;
        }

        auto path = std::filesystem::path(dependency.source);
        if (path.is_relative())
        {
            return path.generic_string();
        }

        const auto relative = path.lexically_relative(workspaceRoot);
        if (!relative.empty())
        {
            return relative.generic_string();
        }
        return path.generic_string();
    }

} // namespace rhodium
