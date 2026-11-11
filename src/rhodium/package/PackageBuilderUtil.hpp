/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */
#pragma once

#include "rhodium/model/DependencyDesc.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;
    class ToolchainSnapshot;
}

namespace rhodium
{
    class BuildUtil
    {
    public:
        struct Context
        {
            std::filesystem::path sourceDir;
            std::filesystem::path buildDir;
            std::filesystem::path artifactDir;
            std::string packageName;
            std::string config;
            std::string platform;
            std::string arch;
            std::string cc;
            std::string cxx;
            std::string linker;
            std::string archiver;
            std::string rc;
            std::string mt;
            std::string cmake;
            std::string ninja;
            std::string git;
            std::filesystem::path workspaceRoot;
            std::map<std::string, std::string> environment;
            ToolchainSnapshot *toolchains = nullptr;
        };

        static bool toolchain(Context &context,
                              const std::string &preferredToolchain = {},
                              const ToolchainRequirements &requirements = {},
                              DiagnosticSink *diagnostics = nullptr);
        static std::string expand(std::string value, const Context &context);
        static std::vector<std::string> expand(const std::vector<std::string> &values, const Context &context);
        static std::map<std::string, std::string> expand(const std::map<std::string, std::string> &values, const Context &context);
        static std::map<std::string, std::string> cmakeOptions(const DependencyDesc &dependency);
        static std::vector<std::string> cmakeArgs(const DependencyDesc &dependency, const Context &context);
        static std::vector<std::string> wrap(const std::filesystem::path &workingDir,
                                             const std::map<std::string, std::string> &env,
                                             const std::string &executable,
                                             const std::vector<std::string> &args);
        static bool materialize(const Context &context,
                                const DependencyArtifacts &requested,
                                DependencyArtifacts &exported,
                                DiagnosticSink &diagnostics);
        static bool markBuilt(const Context &context, DiagnosticSink &diagnostics);
        static bool validate(const Context &context,
                             const DependencyArtifacts &artifacts,
                             DiagnosticSink &diagnostics);
    };
} // namespace rhodium
