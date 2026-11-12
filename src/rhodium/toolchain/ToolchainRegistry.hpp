/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/toolchain/Toolchain.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    struct LocalToolchainConfig
    {
        std::string preferredToolchain;
    };

    enum class ToolchainSource
    {
        AutoDetected,
        GlobalUser,
        Workspace,
        ProjectUser,
    };

    struct ToolchainOverrideRecord
    {
        std::string id;
        ToolchainSource source = ToolchainSource::AutoDetected;
        std::filesystem::path sourceFile;
    };

    struct LoadedToolchainProfile
    {
        ToolchainProfile profile;
        ToolchainSource source = ToolchainSource::AutoDetected;
        std::filesystem::path sourceFile;
        std::vector<ToolchainOverrideRecord> overridden;
    };

    class ToolchainRegistry
    {
    public:
        static const char *name(ToolchainSource source);
        static bool parseSource(const std::string &value, ToolchainSource &source);
        static std::filesystem::path projectPath(const std::filesystem::path &workspaceRoot);
        static std::filesystem::path globalPath();
        static std::vector<LoadedToolchainProfile> sources(const std::filesystem::path &workspaceRoot,
                                                           DiagnosticSink &diagnostics);
        static std::vector<LoadedToolchainProfile> load(const std::filesystem::path &workspaceRoot,
                                                        DiagnosticSink &diagnostics);
        static std::vector<ToolchainProfile> profiles(const std::vector<LoadedToolchainProfile> &profiles);
        static bool writeUser(const std::filesystem::path &workspaceRoot, ToolchainSource source,
                              const std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
        static bool readUser(const std::filesystem::path &workspaceRoot, ToolchainSource source,
                             std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
        static bool write(const std::filesystem::path &path, const std::vector<ToolchainProfile> &profiles,
                          DiagnosticSink &diagnostics);
        static bool read(const std::filesystem::path &path, std::vector<ToolchainProfile> &profiles,
                         DiagnosticSink &diagnostics);
        static std::filesystem::path configPath(const std::filesystem::path &workspaceRoot);
        static bool readConfig(const std::filesystem::path &workspaceRoot, LocalToolchainConfig &config,
                               DiagnosticSink &diagnostics);
        static bool writeConfig(const std::filesystem::path &workspaceRoot, const LocalToolchainConfig &config,
                                DiagnosticSink &diagnostics);
    };

}
