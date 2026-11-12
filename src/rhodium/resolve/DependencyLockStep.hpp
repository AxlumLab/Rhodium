#pragma once

#include "rhodium/export.hpp"

#include "rhodium/config/LockFile.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    class RHODIUM_EXPORT DepLock
    {
    public:
        static LockedPackage *find(LockFile &lockFile, const DependencyDesc &dependency, const std::string &config,
                                   const std::string &toolchainId);
        static std::string source(const std::filesystem::path &workspaceRoot, const DependencyDesc &dependency);
        static void store(LockFile &lockFile, LockedPackage package, const LockedPackage *previous = nullptr);

    private:
        static bool same(const LockedPackage &package, const DependencyDesc &dependency);
        static bool matches(const LockedPackage &package, const DependencyDesc &dependency, const std::string &config,
                            const std::string &toolchainId);
        static bool legacy(const LockedPackage &package, const DependencyDesc &dependency, const std::string &config);
    };

} // namespace rhodium
