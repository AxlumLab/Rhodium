/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/toolchain/Toolchain.hpp"

#include <filesystem>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    class RHODIUM_EXPORT ToolchainDetector
    {
    public:
        static void windows(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
        static void android(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &workspaceRoot,
                            DiagnosticSink &diagnostics);
        static void unixProfiles(std::vector<ToolchainProfile> &profiles);
        static std::filesystem::path findExecutable(const std::string &name);
        static std::filesystem::path findTool(const std::filesystem::path &dir, const std::string &baseName);
        static void windows(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &vsRoot,
                            const std::filesystem::path &windowsSdkRoot, DiagnosticSink &diagnostics);
        static void android(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &sdkRoot,
                            const std::filesystem::path &ndkRoot, DiagnosticSink &diagnostics);
        static std::vector<ToolchainProfile> host(DiagnosticSink &diagnostics);
        static std::vector<ToolchainProfile> host(const std::filesystem::path &workspaceRoot,
                                                  DiagnosticSink &diagnostics);
        static bool writeHost(const std::filesystem::path &workspaceRoot, std::vector<ToolchainProfile> &profiles,
                              DiagnosticSink &diagnostics);
        static std::filesystem::path cachePath(const std::filesystem::path &workspaceRoot);
    };

}
