/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

namespace rhodium
{

    struct DependencyArtifacts
    {
        std::string mode = "install";
        std::vector<std::string> includeDirs;
        std::vector<std::string> libDirs;
        std::vector<std::string> binDirs;
        std::vector<std::string> binFiles;
        std::vector<std::string> libs;
        std::vector<std::string> libFiles;
        std::map<std::string, std::vector<std::string>> libFilesByConfig;
        std::vector<std::string> defines;
        std::vector<std::string> systemLibs;
        std::vector<std::string> frameworks;
    };

    struct ToolchainRequirements
    {
        std::string family;
        std::string abi;
        std::string binaryFormat;
    };

    struct CMakeBuildConfig
    {
        std::string generator = "Ninja";
        std::string buildType;
        std::map<std::string, std::string> options;
        std::vector<std::string> configureArgs;
        std::vector<std::string> buildArgs;
        std::vector<std::string> buildTargets;
        std::vector<std::string> installArgs;
        std::map<std::string, std::string> env;
        std::string toolchainFile;
        bool install = true;
        std::string installTarget;
    };

    struct MakeBuildConfig
    {
        std::string executable = "make";
        int jobs = 0;
        std::vector<std::string> args;
        std::vector<std::string> targets;
        std::vector<std::string> installTargets;
        std::vector<std::string> installArgs;
        std::map<std::string, std::string> env;
    };

    struct ConfigureBuildConfig
    {
        std::string script = "./configure";
        std::vector<std::string> args;
        std::map<std::string, std::string> env;
    };

    struct CustomBuildCommands
    {
        std::vector<std::string> configure;
        std::vector<std::string> build;
        std::vector<std::string> install;
    };

    struct DependencyDesc
    {
        std::string name;
        std::string source;
        std::string sourceType = "git";
        std::vector<std::string> mirrors;  // Fallback sources
        std::string sha256;
        std::string resolvedArchiveHash;
        std::string ref;
        std::string subdir = ".";
        int stripComponents = 0;
        std::vector<std::string> patches;
        std::vector<std::string> patchHashes;
        std::string buildType = "header_only";
        std::string linkage = "default";
        std::string runtime = "default";
        std::string pic = "default";
        std::map<std::string, std::string> buildOptions;
        CMakeBuildConfig cmake;
        MakeBuildConfig make;
        ConfigureBuildConfig configure;
        CustomBuildCommands commands;
        DependencyArtifacts artifacts;
        ToolchainRequirements toolchainRequirements;
        std::vector<std::string> dependencies;  // Transitive dependencies
        bool exportDefines = false;  // Export defines to consuming projects
        bool overridden = false;
        std::string overridePath;
        std::string overrideSource;
        std::string originalSource;
        std::string originalSourceType;
        std::string originalRef;
        bool local = false;
    };

} // namespace rhodium
