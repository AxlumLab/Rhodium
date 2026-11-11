/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/toolchain/ToolchainDetector.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class ToolProbe
    {
    public:
        static std::vector<std::string> paths(const std::string &value);
        static std::string env(const char *key);
        static std::string trim(std::string value);
        static std::string arch();
        static bool newer(const std::string &lhs, const std::string &rhs);
        static std::filesystem::path exe(std::string name);
        static std::vector<std::string> exeNames(const std::string &baseName);
        static std::filesystem::path inDir(const std::filesystem::path &dir, const std::string &baseName);
        static void appendDir(std::vector<std::filesystem::path> &values, const std::filesystem::path &path);
        static std::filesystem::path newest(const std::filesystem::path &root);
        static std::filesystem::path newestOf(const std::vector<std::filesystem::path> &roots);
        static std::filesystem::path programFiles();
        static std::filesystem::path programFilesX86();
        static void addTools(ToolchainProfile &profile);
        static void appendEnvDir(std::vector<std::filesystem::path> &paths, const char *key);
        static void appendEnvCandidate(std::vector<std::filesystem::path> &paths, const char *key);
        static void appendUnique(std::vector<std::filesystem::path> &paths, const std::filesystem::path &path);
        static std::filesystem::path home();
        static bool intAtLeast(const std::string &value, int minimum);
        static std::filesystem::path binExe(const std::filesystem::path &root, const std::string &name);
        static std::filesystem::path resolveExe(const std::filesystem::path &candidate, const std::string &fallback);
        static std::filesystem::path gradle();
        static void addPath(ToolchainProfile &profile);
        static std::string join(const std::vector<std::filesystem::path> &paths);
        static std::filesystem::path androidPrebuilt(const std::filesystem::path &ndkRoot);
        static void complete(ToolchainProfile &profile);

    private:
        static bool hasPart(const std::filesystem::path &path, const std::string &part);
        static bool hasMsvcLib(const ToolchainProfile &profile);
        static bool hasUcrtLib(const ToolchainProfile &profile);
        static bool hasUmLib(const ToolchainProfile &profile);
        static bool hasInclude(const ToolchainProfile &profile, const std::string &part);
    };

    void addWindowsProfiles(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    void addAndroidProfiles(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics);
    void addUnixProfiles(std::vector<ToolchainProfile> &profiles);
}
