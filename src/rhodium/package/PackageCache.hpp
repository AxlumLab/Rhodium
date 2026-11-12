/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/DependencyDesc.hpp"

#include <filesystem>

namespace rhodium
{

    class RHODIUM_EXPORT PackageCache
    {
    public:
        explicit PackageCache(std::filesystem::path root);

        const std::filesystem::path &root() const;
        std::filesystem::path source(const DependencyDesc &dependency) const;
        std::filesystem::path path(const DependencyDesc &dependency) const;
        std::filesystem::path path(const DependencyDesc &dependency, const std::string &commitOrRef) const;
        std::filesystem::path sourceRoot(const DependencyDesc &dependency) const;
        std::filesystem::path build(const DependencyDesc &dependency, const std::string &buildKey) const;
        std::filesystem::path artifact(const DependencyDesc &dependency, const std::string &commit) const;
        bool mkdir(std::string *error = nullptr) const;

    private:
        static std::string sourceName(const DependencyDesc &dependency);

        std::filesystem::path root_;
    };

} // namespace rhodium
