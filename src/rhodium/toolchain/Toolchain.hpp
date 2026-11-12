/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <rhodium/common.hpp>

#include <string>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace rhodium
{
    struct ToolchainProfile
    {
        std::string id;
        std::string platform;
        std::string hostArch;
        std::string targetArch;
        std::string compilerKind;
        std::string sdkKind;

        std::filesystem::path compiler;
        std::filesystem::path cxxCompiler;
        std::filesystem::path linker;
        std::filesystem::path archiver;
        std::filesystem::path resourceCompiler;
        std::filesystem::path manifestTool;
        std::filesystem::path cmake;
        std::filesystem::path ninja;
        std::filesystem::path git;
        std::filesystem::path strip;
        std::filesystem::path ranlib;
        std::filesystem::path libtool;
        std::filesystem::path lipo;
        std::filesystem::path codesign;
        std::filesystem::path adb;
        std::filesystem::path java;
        std::filesystem::path javac;
        std::filesystem::path gradle;

        std::string compilerVersion;
        std::filesystem::path sdkRoot;
        std::filesystem::path sysroot;
        std::string sdkVersion;
        std::filesystem::path androidSdkRoot;
        std::filesystem::path androidNdkRoot;
        std::filesystem::path jdkRoot;
        std::string androidApi;
        std::string androidAbi;
        std::string targetTriple;
        std::string deploymentTarget;

        std::vector<std::filesystem::path> systemIncludeDirs;
        std::vector<std::filesystem::path> systemLibDirs;
        std::vector<std::filesystem::path> binaryDirs;
        std::map<std::string, std::string> environment;
        bool complete = false;
        std::vector<std::string> missing;
    };

    class RHODIUM_EXPORT Toolchain
    {
    public:
        static std::string contentHash(const ToolchainProfile &profile, const std::string &config);

        virtual ~Toolchain() = default;
        virtual std::string name() const = 0;
    };

} // namespace rhodium
