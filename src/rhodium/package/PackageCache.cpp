/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/PackageCache.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Hash.hpp"
#include "rhodium/core/Path.hpp"

#include <algorithm>

namespace rhodium
{
    std::string PackageCache::sourceName(const DependencyDesc &dependency)
    {
        if (!dependency.name.empty())
        {
            return Path::sanitizePackagePath(dependency.name);
        }

        std::string source = dependency.source;
        std::replace(source.begin(), source.end(), '\\', '/');
        while (!source.empty() && source.back() == '/')
        {
            source.pop_back();
        }

        const auto slash = source.find_last_of('/');
        auto name = slash == std::string::npos ? source : source.substr(slash + 1);
        if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
        {
            name.resize(name.size() - 4);
        }
        return Path::sanitizePackagePath(name.empty() ? "source" : name);
    }

    PackageCache::PackageCache(std::filesystem::path root)
        : root_(std::move(root))
    {
    }

    const std::filesystem::path &PackageCache::root() const
    {
        return root_;
    }

    std::filesystem::path PackageCache::path(const DependencyDesc &dependency) const
    {
        return source(dependency);
    }

    std::filesystem::path PackageCache::source(const DependencyDesc &dependency) const
    {
        if (dependency.local)
        {
            return std::filesystem::path(dependency.source);
        }
        const auto version = dependency.ref.empty() ? "head" : dependency.ref;
        return path(dependency, version);
    }

    std::filesystem::path PackageCache::path(const DependencyDesc &dependency, const std::string &commitOrRef) const
    {
        if (dependency.local)
        {
            return std::filesystem::path(dependency.source);
        }
        return sourceRoot(dependency) / Path::sanitizePackagePath(commitOrRef.empty() ? "head" : commitOrRef);
    }

    std::filesystem::path PackageCache::sourceRoot(const DependencyDesc &dependency) const
    {
        return root_ / sourceName(dependency) / Hash::xxhash64Hex(dependency.source);
    }

    std::filesystem::path PackageCache::build(const DependencyDesc &dependency, const std::string &buildKey) const
    {
        return root_.parent_path() / "build-packages" / Path::sanitizePackagePath(dependency.name) / Path::sanitizePackagePath(buildKey);
    }

    std::filesystem::path PackageCache::artifact(const DependencyDesc &dependency, const std::string &commit) const
    {
        return root_.parent_path() / "artifacts" / Path::sanitizePackagePath(dependency.name) / Path::sanitizePackagePath(commit);
    }

    bool PackageCache::mkdir(std::string *error) const
    {
        return File::mkdir(root_, error);
    }

} // namespace rhodium
