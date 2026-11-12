/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/cli/CommandContext.hpp"
#include "rhodium/config/DependencyOverrideFile.hpp"
#include "rhodium/config/LockFile.hpp"
#include "rhodium/diagnostics/ConsoleDiagnosticSink.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/model/BuildModel.hpp"
#include "rhodium/toolchain/Toolchain.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class CliWorkspace
    {
    public:
        static bool overrides(const BuildModel &model, const std::filesystem::path &workspaceRoot,
                              const std::vector<std::string> &cliOverrides, DependencyOverrideSet &overrides,
                              DiagnosticSink &diagnostics);
        static std::string config(const BuildModel &model, const std::string &configOption,
                                  DiagnosticSink &diagnostics);
        static std::string missingDependencies(const std::string &config, const std::string &toolchainId);
        static std::string runtime(const LockFile &lockFile, const std::string &config, const std::string &toolchainId);
        static std::string linkage(const LockFile &lockFile, const std::string &config, const std::string &toolchainId);

        static bool selectToolchain(const std::filesystem::path &workspaceRoot, const std::string &toolchainId,
                                    ToolchainProfile &selectedToolchain, DiagnosticSink &diagnostics);
        static std::string toolchain(const std::filesystem::path &workspaceRoot, const std::string &toolchainId,
                                     DiagnosticSink &diagnostics);
        static std::filesystem::path manifest(const std::filesystem::path &root);
        static bool load(const std::filesystem::path &root, BuildModel &model, ConsoleDiagnosticSink &diagnostics);
    };

    class SdkDoctor
    {
    public:
        static bool print(const std::filesystem::path &workspaceRoot);
    };

    struct RestoreRequest
    {
        std::string preferredToolchain;
        std::string configOption;
        std::vector<std::string> cliOverrides;
        bool locked = false;
        std::string packageName;
        bool buildOnly = false;
        bool exportOnly = false;
        bool rebuild = false;
        std::string updatePackage;
        bool updateAll = false;
    };

    struct CleanOptions
    {
        bool all = false;
        bool artifacts = false;
        bool sources = false;
        bool lock = false;
        std::string packageName;
    };

    struct PackageGcOptions
    {
        bool apply = false;
        std::string packageFilter;
        bool builds = false;
        bool sources = false;
        std::string preferredToolchain;
        std::string configOption;
    };

    struct UpdateRequest
    {
        std::string packageName;
        bool updateAll = false;
        std::string preferredToolchain;
        std::string configOption;
        std::vector<std::string> cliOverrides;
    };

    class Commands
    {
    public:
        static int doctor(const std::filesystem::path &workspaceRoot, bool sdkDetails);
        static int load(const std::filesystem::path &workspaceRoot);
        static int dumpModel(const std::filesystem::path &workspaceRoot);
        static int deps(const std::filesystem::path &workspaceRoot);
        static int generate(const std::filesystem::path &workspaceRoot, const std::string &toolchainId,
                            const std::string &configOption, const std::filesystem::path &outputDir,
                            const std::vector<std::string> &cliOverrides);
        static int restore(const std::filesystem::path &workspaceRoot, const RestoreRequest &request);
        static int update(const std::filesystem::path &workspaceRoot, const UpdateRequest &request);
        static int packages(const std::filesystem::path &workspaceRoot, bool verbose,
                            const std::vector<std::string> &cliOverrides);
        static int packagesGc(const std::filesystem::path &workspaceRoot, const PackageGcOptions &request);
        static int tree(const std::filesystem::path &workspaceRoot);
        static int clean(const std::filesystem::path &workspaceRoot, const CleanOptions &options);
        static int overrideList(const std::filesystem::path &workspaceRoot);
        static int overrideSet(const std::filesystem::path &workspaceRoot, const std::string &name,
                               const std::string &path);
        static int overrideRemove(const std::filesystem::path &workspaceRoot, const std::string &name);
        static int overrideClear(const std::filesystem::path &workspaceRoot);

        static int sdkDetect(const std::filesystem::path &workspaceRoot);
        static int sdkList(const std::filesystem::path &workspaceRoot, bool refresh, const std::string &sourceFilter);
        static int sdkShow(const std::filesystem::path &workspaceRoot, const std::string &id);
        static int sdkAdd(const std::filesystem::path &workspaceRoot, const SdkAddOptions &options);
        static int sdkRemove(const std::filesystem::path &workspaceRoot, const std::string &id,
                             const std::string &sourceName);
        static int sdkDump(const std::filesystem::path &workspaceRoot);
        static int sdkSelect(const std::filesystem::path &workspaceRoot, const std::string &toolchainId);
        static int sdkClear(const std::filesystem::path &workspaceRoot);
    };

}
