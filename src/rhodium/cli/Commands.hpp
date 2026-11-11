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
        static std::filesystem::path manifest(const std::filesystem::path &root);
        static bool load(const std::filesystem::path &root, BuildModel &model, ConsoleDiagnosticSink &diagnostics);
    };

    bool printSdkDoctor(const std::filesystem::path &workspaceRoot);

    bool loadEffectiveDependencyOverrides(const BuildModel &model,
                                          const std::filesystem::path &workspaceRoot,
                                          const std::vector<std::string> &cliOverrides,
                                          DependencyOverrideSet &overrides,
                                          DiagnosticSink &diagnostics);
    std::string resolveCommandConfig(const BuildModel &model, const std::string &configOption, DiagnosticSink &diagnostics);
    std::string dependenciesNotRestoredMessage(const std::string &config, const std::string &toolchainId);
    std::string inferRuntimeMetadata(const LockFile &lockFile, const std::string &config, const std::string &toolchainId);
    std::string inferLinkageMetadata(const LockFile &lockFile, const std::string &config, const std::string &toolchainId);
    std::string resolveCommandConfigWithState(const BuildModel &model,
                                              const std::filesystem::path &workspaceRoot,
                                              const std::string &configOption,
                                              DiagnosticSink &diagnostics);
    bool selectToolchainForRestore(const std::filesystem::path &workspaceRoot,
                                   const std::string &toolchainId,
                                   ToolchainProfile &selectedToolchain,
                                   DiagnosticSink &diagnostics);
    std::string preferredToolchainForCommand(const std::filesystem::path &workspaceRoot, const std::string &toolchainId, DiagnosticSink &diagnostics);

    class Commands
    {
    public:
        static int doctor(const std::filesystem::path &workspaceRoot, bool sdkDetails);
        static int load(const std::filesystem::path &workspaceRoot);
        static int dumpModel(const std::filesystem::path &workspaceRoot);
        static int deps(const std::filesystem::path &workspaceRoot);
        static int generate(const std::filesystem::path &workspaceRoot,
                            const std::string &toolchainId,
                            const std::string &configOption,
                            const std::filesystem::path &outputDir,
                            const std::vector<std::string> &cliOverrides);
        static int restore(const std::filesystem::path &workspaceRoot,
                           const std::string &preferredToolchain,
                           const std::string &configOption,
                           const std::vector<std::string> &cliOverrides,
                           bool locked,
                           const std::string &packageName,
                           bool buildOnly,
                           bool exportOnly,
                           bool rebuild,
                           const std::string &updatePackage,
                           bool updateAll);
        static int update(const std::filesystem::path &workspaceRoot,
                          const std::string &packageName,
                          bool updateAll,
                          const std::string &preferredToolchain,
                          const std::string &configOption,
                          const std::vector<std::string> &cliOverrides);
        static int packages(const std::filesystem::path &workspaceRoot,
                            bool verbose,
                            const std::vector<std::string> &cliOverrides);
        static int packagesGc(const std::filesystem::path &workspaceRoot,
                              bool apply,
                              const std::string &packageName,
                              bool includeBuilds,
                              bool includeSources,
                              const std::string &preferredToolchain,
                              const std::string &configOption);
        static int tree(const std::filesystem::path &workspaceRoot);
        static int clean(const std::filesystem::path &workspaceRoot,
                         bool all,
                         bool artifacts,
                         bool sources,
                         bool lock,
                         const std::string &packageName);
        static int overrideList(const std::filesystem::path &workspaceRoot);
        static int overrideSet(const std::filesystem::path &workspaceRoot, const std::string &name, const std::string &path);
        static int overrideRemove(const std::filesystem::path &workspaceRoot, const std::string &name);
        static int overrideClear(const std::filesystem::path &workspaceRoot);

        static int sdkDetect(const std::filesystem::path &workspaceRoot);
        static int sdkList(const std::filesystem::path &workspaceRoot, bool refresh, const std::string &sourceFilter);
        static int sdkShow(const std::filesystem::path &workspaceRoot, const std::string &id);
        static int sdkAdd(const std::filesystem::path &workspaceRoot, const SdkAddOptions &options);
        static int sdkRemove(const std::filesystem::path &workspaceRoot, const std::string &id, const std::string &sourceName);
        static int sdkDump(const std::filesystem::path &workspaceRoot);
        static int sdkSelect(const std::filesystem::path &workspaceRoot, const std::string &toolchainId);
        static int sdkClear(const std::filesystem::path &workspaceRoot);
    };

}
