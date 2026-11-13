/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

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

    class RHODIUM_EXPORT ArchiveExtractor
    {
    public:
        bool available() const;
        bool extract(const std::filesystem::path &archivePath,
                     const std::filesystem::path &destination,
                     int stripComponents,
                     DiagnosticSink &diagnostics) const;
    };

    class RHODIUM_EXPORT ArchiveFetcher
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
