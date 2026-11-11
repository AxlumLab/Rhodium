/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/cli/Commands.hpp"

#include "rhodium/core/Path.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/config/DependencyOverrideFile.hpp"
#include "rhodium/config/LockFile.hpp"
#include "rhodium/resolve/DependencyResolver.hpp"
#include "rhodium/script/ManifestLoader.hpp"
#include "rhodium/toolchain/Toolchain.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rhodium
{
    std::filesystem::path CliWorkspace::manifest(const std::filesystem::path &workspaceRoot)
    {
        return Path::manifest(workspaceRoot);
    }

    bool CliWorkspace::load(const std::filesystem::path &workspaceRoot, BuildModel &model, ConsoleDiagnosticSink &diagnostics)
    {
        ManifestLoader loader;
        auto result = loader.load(manifest(workspaceRoot), diagnostics);
        if (!result.ok)
        {
            return false;
        }
        model = std::move(result.model);
        return true;
    }

    std::string preferredToolchainForCommand(const std::filesystem::path &workspaceRoot, const std::string &toolchainId, DiagnosticSink &diagnostics);
    bool selectToolchainForRestore(const std::filesystem::path &workspaceRoot,
                                   const std::string &toolchainId,
                                   ToolchainProfile &selectedToolchain,
                                   DiagnosticSink &diagnostics);

    bool loadEffectiveDependencyOverrides(const BuildModel &model,
                                          const std::filesystem::path &workspaceRoot,
                                          const std::vector<std::string> &cliOverrides,
                                          DependencyOverrideSet &overrides,
                                          DiagnosticSink &diagnostics)
    {
        return DependencyOverrides::make(model, workspaceRoot, cliOverrides, overrides, diagnostics);
    }

    std::string resolveCommandConfig(const BuildModel &model, const std::string &configOption, DiagnosticSink &diagnostics)
    {
        const auto config = BuildConfig::normalize(configOption, model.workspace.defaultConfig);
        return BuildConfig::validate(config, diagnostics) ? config : std::string{};
    }

    std::string dependenciesNotRestoredMessage(const std::string &config, const std::string &toolchainId)
    {
        return "dependencies are not restored for config=" + config + " toolchain=" + toolchainId +
               "\nrun:\n  rhodium restore --config " + config + " --toolchain " + toolchainId +
               "\nor select a default toolchain:\n  rhodium sdk select " + toolchainId;
    }

    std::string inferPackageMetadataValue(const LockFile &lockFile,
                                          const std::string &config,
                                          const std::string &toolchainId,
                                          const std::string LockedPackage::*field)
    {
        std::string value;
        for (const auto &package : lockFile.packages)
        {
            if (BuildConfig::normalize(package.config) != BuildConfig::normalize(config) || package.toolchainId != toolchainId)
            {
                continue;
            }
            const auto &candidate = package.*field;
            if (candidate.empty() || candidate == "default")
            {
                continue;
            }
            if (value.empty())
            {
                value = candidate;
            }
            else if (value != candidate)
            {
                return "mixed";
            }
        }
        return value;
    }

    std::string inferRuntimeMetadata(const LockFile &lockFile, const std::string &config, const std::string &toolchainId)
    {
        return inferPackageMetadataValue(lockFile, config, toolchainId, &LockedPackage::runtime);
    }

    std::string inferLinkageMetadata(const LockFile &lockFile, const std::string &config, const std::string &toolchainId)
    {
        return inferPackageMetadataValue(lockFile, config, toolchainId, &LockedPackage::linkage);
    }

    std::string resolveCommandConfigWithState(const BuildModel &model,
                                              const std::filesystem::path &workspaceRoot,
                                              const std::string &configOption,
                                              DiagnosticSink &diagnostics)
    {
        (void)workspaceRoot;
        return resolveCommandConfig(model, configOption, diagnostics);
    }

    bool isCompleteToolchainForBuild(const ToolchainProfile &profile, DiagnosticSink &diagnostics, bool allowIncomplete)
    {
        if (profile.complete)
        {
            return true;
        }
        if (allowIncomplete)
        {
            diagnostics.warning("using incomplete toolchain: " + profile.id);
            return true;
        }
        std::ostringstream missing;
        for (std::size_t i = 0; i < profile.missing.size(); ++i)
        {
            if (i != 0)
            {
                missing << ", ";
            }
            missing << profile.missing[i];
        }
        diagnostics.error("toolchain " + profile.id + " is incomplete\nmissing: " +
                          (profile.missing.empty() ? std::string("-") : missing.str()) +
                          "\nrun:\n  rhodium sdk doctor");
        return false;
    }

    bool selectToolchainForRestore(const std::filesystem::path &workspaceRoot,
                                   const std::string &toolchainId,
                                   ToolchainProfile &selectedToolchain,
                                   DiagnosticSink &diagnostics)
    {
        const auto loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
        const auto profiles = unwrapToolchainProfiles(loadedProfiles);
        const auto found = std::find_if(profiles.begin(), profiles.end(), [&](const ToolchainProfile &profile)
                                        { return profile.id == toolchainId; });
        if (found == profiles.end())
        {
            diagnostics.error("requested toolchain was not found: " + toolchainId +
                              "\nrun:\n  rhodium sdk list --refresh");
            return false;
        }
        selectedToolchain = *found;
        return isCompleteToolchainForBuild(selectedToolchain, diagnostics, false);
    }

    std::string preferredToolchainForCommand(const std::filesystem::path &workspaceRoot, const std::string &toolchainId, DiagnosticSink &diagnostics)
    {
        if (!toolchainId.empty())
        {
            return toolchainId;
        }
        LocalToolchainConfig config;
        if (!readLocalToolchainConfig(workspaceRoot, config, diagnostics))
        {
            return {};
        }
        if (config.preferredToolchain.empty())
        {
            diagnostics.error("no toolchain selected; pass --toolchain <id> or run:\n  rhodium sdk select <id>");
            return {};
        }
        return config.preferredToolchain;
    }

} // namespace rhodium
