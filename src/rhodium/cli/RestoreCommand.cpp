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
    namespace
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
    }

    namespace
    {
        class RestoreTimer final
        {
        public:
            RestoreTimer(DiagnosticSink &diagnostics, std::chrono::steady_clock::time_point start)
                : diagnostics(diagnostics), start(start)
            {
            }

            int failed() const
            {
                diagnostics.error("restore failed (took " + elapsed() + ")");
                return 1;
            }

            void completed(bool update) const
            {
                diagnostics.success((update ? "update completed (took " : "restore completed (took ") + elapsed() +
                                    ")");
            }

        private:
            std::string elapsed() const
            {
                return RestoreCli::duration(
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start));
            }

            DiagnosticSink &diagnostics;
            std::chrono::steady_clock::time_point start;
        };
    }

    int Commands::restore(const std::filesystem::path &workspaceRoot, const RestoreRequest &request)
    {
        const auto &[preferredToolchain, configOption, cliOverrides, locked, packageName, buildOnly, exportOnly,
                     rebuild, updatePackage, updateAll] = request;
        const auto startTime = std::chrono::steady_clock::now();

        ConsoleDiagnosticSink diagnostics;
        RestoreTimer timer(diagnostics, startTime);
        if (buildOnly && exportOnly)
        {
            diagnostics.error("restore --build-only and --export-only cannot be used together");
            return 1;
        }
        const auto effectiveToolchain = CliWorkspace::toolchain(workspaceRoot, preferredToolchain, diagnostics);
        if (effectiveToolchain.empty())
        {
            return timer.failed();
        }
        ToolchainProfile selectedToolchain;
        if (!CliWorkspace::selectToolchain(workspaceRoot, effectiveToolchain, selectedToolchain, diagnostics))
        {
            return timer.failed();
        }
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            return timer.failed();
        }
        DependencyOverrideSet overrides;
        if (!CliWorkspace::overrides(model, workspaceRoot, cliOverrides, overrides, diagnostics))
        {
            return timer.failed();
        }
        const auto config = CliWorkspace::config(model, configOption, diagnostics);
        if (config.empty())
        {
            return timer.failed();
        }

        GitFetcher git;
        if (!git.initialize(diagnostics))
        {
            return timer.failed();
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
        options.toolchainContentHash = Toolchain::contentHash(selectedToolchain, config);
        if (!resolver.restore(model, workspaceRoot, diagnostics,
                              DependencyRestoreRequest{.preferredToolchain = effectiveToolchain,
                                                       .config = config,
                                                       .overrides = overrides,
                                                       .options = options}))
        {
            return timer.failed();
        }

        timer.completed(updateAll || !updatePackage.empty());
        return 0;
    }

} // namespace rhodium
