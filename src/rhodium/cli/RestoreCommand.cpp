/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/cli/Commands.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/package/GitFetcher.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/resolve/DependencyResolver.hpp"

#include <fmt/format.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class RestoreCli final
    {
    public:
        static std::string duration(std::chrono::milliseconds value)
        {
            auto ms = value.count();
            if (ms < 1000)
            {
                return std::to_string(ms) + "ms";
            }
            else if (ms < 60000)
            {
                double seconds = ms / 1000.0;
                return fmt::format("{:.1f}s", seconds);
            }
            else
            {
                int minutes = static_cast<int>(ms / 60000);
                int seconds = static_cast<int>((ms % 60000) / 1000);
                return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
            }
        }
    };

    int Commands::restore(const std::filesystem::path &workspaceRoot,
                          const std::string &preferredToolchain,
                          const std::string &configOption,
                          const std::vector<std::string> &cliOverrides,
                          bool locked,
                          const std::string &packageName,
                          bool buildOnly,
                          bool exportOnly,
                          bool rebuild,
                          const std::string &updatePackage,
                          bool updateAll)
    {
        auto startTime = std::chrono::steady_clock::now();

        ConsoleDiagnosticSink diagnostics;
        if (buildOnly && exportOnly)
        {
            diagnostics.error("restore --build-only and --export-only cannot be used together");
            return 1;
        }
        const auto effectiveToolchain = preferredToolchainForCommand(workspaceRoot, preferredToolchain, diagnostics);
        if (effectiveToolchain.empty())
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }
        ToolchainProfile selectedToolchain;
        if (!selectToolchainForRestore(workspaceRoot, effectiveToolchain, selectedToolchain, diagnostics))
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }
        DependencyOverrideSet overrides;
        if (!loadEffectiveDependencyOverrides(model, workspaceRoot, cliOverrides, overrides, diagnostics))
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }
        const auto config = resolveCommandConfigWithState(model, workspaceRoot, configOption, diagnostics);
        if (config.empty())
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }

        GitFetcher git;
        if (!git.initialize(diagnostics))
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }

        DependencyResolver resolver;
        DependencyRestoreOptions options;
        options.locked = locked;
        options.packageName = packageName;
        options.buildOnly = buildOnly;
        options.exportOnly = exportOnly;
        options.rebuild = rebuild;
        options.updatePackage = updatePackage;
        options.updateAll = updateAll;
        options.toolchainContentHash = toolchainContentHash(selectedToolchain, config);
        if (!resolver.restore(model, workspaceRoot, diagnostics, effectiveToolchain, config, overrides, options))
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            diagnostics.error("restore failed (took " + RestoreCli::duration(duration) + ")");
            return 1;
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        diagnostics.success((updateAll || !updatePackage.empty()) ? "update completed (took " + RestoreCli::duration(duration) + ")" : "restore completed (took " + RestoreCli::duration(duration) + ")");
        return 0;
    }

} // namespace rhodium
