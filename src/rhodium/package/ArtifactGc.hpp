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

    class ArtifactGc
    {
    public:
        static bool runArtifacts(const LockFile &lockFile,
                                 const std::filesystem::path &workspaceRoot,
                                 bool apply,
                                 const std::string &packageFilter,
                                 DiagnosticSink &diagnostics);
        static bool runBuildCaches(const LockFile &lockFile,
                                   const std::filesystem::path &workspaceRoot,
                                   const std::string &config,
                                   const std::string &toolchainId,
                                   bool apply,
                                   const std::string &packageFilter,
                                   DiagnosticSink &diagnostics);
        static bool runSourceCaches(const LockFile &lockFile,
                                    const std::filesystem::path &workspaceRoot,
                                    bool apply,
                                    const std::string &packageFilter,
                                    DiagnosticSink &diagnostics);
    };

}
