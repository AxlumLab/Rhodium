/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/model/DependencyDesc.hpp"

#include <filesystem>

namespace rhodium
{

    class DiagnosticSink;

    class GitFetcher
    {
    public:
        static bool cachedOrigin(const std::filesystem::path &repository, std::string &url);
        static std::filesystem::path localPath(const std::string &source, const std::filesystem::path &workspaceRoot);
        bool initialize(DiagnosticSink &diagnostics) const;
        bool fetch(const DependencyDesc &dependency, const std::filesystem::path &destination, DiagnosticSink &diagnostics) const;
        bool fetch(const DependencyDesc &dependency, const std::filesystem::path &destination, std::string &commit, DiagnosticSink &diagnostics, bool refresh = false) const;
    };

} // namespace rhodium
