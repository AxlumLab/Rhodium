/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/toolchain/Toolchain.hpp"

#include <filesystem>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    std::filesystem::path findExecutableOnPath(const std::string &name);
    std::filesystem::path findToolInDir(const std::filesystem::path &dir, const std::string &baseName);
    void addWindowsMsvcProfilesFromRoots(std::vector<ToolchainProfile> &profiles,
                                         const std::filesystem::path &vsRoot,
                                         const std::filesystem::path &windowsSdkRoot,
                                         DiagnosticSink &diagnostics);
    void addAndroidProfilesFromRoots(std::vector<ToolchainProfile> &profiles,
                                     const std::filesystem::path &sdkRoot,
                                     const std::filesystem::path &ndkRoot,
                                     DiagnosticSink &diagnostics);
    std::vector<ToolchainProfile> detectHostToolchains(DiagnosticSink &diagnostics);
    std::vector<ToolchainProfile> detectHostToolchains(const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics);
    bool detectAndWriteHostToolchains(const std::filesystem::path &workspaceRoot, std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    std::filesystem::path hostToolchainCachePath(const std::filesystem::path &workspaceRoot);
}
