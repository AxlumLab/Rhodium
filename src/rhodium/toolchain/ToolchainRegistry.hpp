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

    const char *toString(ToolchainSource source);
    bool parseToolchainSource(const std::string &value, ToolchainSource &source);
    std::filesystem::path projectUserToolchainsPath(const std::filesystem::path &workspaceRoot);
    std::filesystem::path globalUserToolchainsPath();
    std::vector<LoadedToolchainProfile> loadToolchainSources(const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics);
    std::vector<LoadedToolchainProfile> loadMergedToolchains(const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics);
    std::vector<ToolchainProfile> unwrapToolchainProfiles(const std::vector<LoadedToolchainProfile> &profiles);
    bool writeUserToolchainProfiles(const std::filesystem::path &workspaceRoot, ToolchainSource source, const std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    bool readUserToolchainProfiles(const std::filesystem::path &workspaceRoot, ToolchainSource source, std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    bool writeToolchainProfiles(const std::filesystem::path &path, const std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    bool readToolchainProfiles(const std::filesystem::path &path, std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    std::filesystem::path localToolchainConfigPath(const std::filesystem::path &workspaceRoot);
    bool readLocalToolchainConfig(const std::filesystem::path &workspaceRoot, LocalToolchainConfig &config, DiagnosticSink &diagnostics);
    bool writeLocalToolchainConfig(const std::filesystem::path &workspaceRoot, const LocalToolchainConfig &config, DiagnosticSink &diagnostics);
}
