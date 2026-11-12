/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    struct PatchOptions
    {
        std::vector<std::string> patches;
        int stripComponents = 0;
    };

    class PatchApplier
    {
    public:
        bool apply(const std::filesystem::path &sourceDir, const std::filesystem::path &workspaceRoot,
                   const PatchOptions &options, std::vector<std::string> &patchHashes,
                   DiagnosticSink &diagnostics) const;

    private:
        static std::filesystem::path path(const std::filesystem::path &workspaceRoot, const std::string &patch);
        static std::string quote(const std::filesystem::path &path);
        static bool git(const std::filesystem::path &sourceDir, const std::filesystem::path &patch,
                        const std::vector<std::string> &stripArgs, DiagnosticSink &diagnostics, std::string &lastError);
        static bool patch(const std::filesystem::path &sourceDir, const std::filesystem::path &patch,
                          DiagnosticSink &diagnostics, const std::string &lastGitError);
    };
}
