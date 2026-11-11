/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/model/DependencyDesc.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    struct ArchiveFetchResult
    {
        std::filesystem::path archivePath;
        std::filesystem::path sourceDir;
        std::string archiveHash;
        std::vector<std::string> patchHashes;
    };

    class ArchiveExtractor
    {
    public:
        bool available() const;
        bool extract(const std::filesystem::path &archivePath,
                     const std::filesystem::path &destination,
                     int stripComponents,
                     DiagnosticSink &diagnostics) const;
    };

    class ArchiveFetcher
    {
    public:
        bool available() const;
        bool fetchAndExtract(const DependencyDesc &dependency,
                             const std::filesystem::path &workspaceRoot,
                             const std::filesystem::path &cacheRoot,
                             ArchiveFetchResult &result,
                             DiagnosticSink &diagnostics) const;
    };
}
