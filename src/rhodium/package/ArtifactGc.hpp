/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/config/LockFile.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{
    class DiagnosticSink;

    struct BuildCacheGcOptions
    {
        std::string config;
        std::string toolchainId;
        bool apply = false;
        std::string packageFilter;
    };

    class ArtifactGc
    {
    public:
        static bool runArtifacts(const LockFile &lockFile, const std::filesystem::path &workspaceRoot, bool apply,
                                 const std::string &packageFilter, DiagnosticSink &diagnostics);
        static bool runBuildCaches(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                   const BuildCacheGcOptions &request, DiagnosticSink &diagnostics);
        static bool runSourceCaches(const LockFile &lockFile, const std::filesystem::path &workspaceRoot, bool apply,
                                    const std::string &packageFilter, DiagnosticSink &diagnostics);
    };

}
