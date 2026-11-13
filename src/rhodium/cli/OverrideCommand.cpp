/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/cli/Commands.hpp"

#include "rhodium/config/DependencyOverrideFile.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Logger.hpp"
#include "rhodium/core/Path.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{
    int Commands::overrideList(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        LocalDependencyOverrides overrides;
        if (!DependencyOverrides::read(workspaceRoot, overrides, diagnostics))
        {
            return 1;
        }
        if (overrides.paths.empty())
        {
            LogInfo("overrides: none");
            return 0;
        }
        for (const auto &[name, path] : overrides.paths)
        {
            LogInfo("{} -> {}", name, path);
        }
        return 0;
    }

    int Commands::overrideSet(const std::filesystem::path &workspaceRoot, const std::string &name,
                              const std::string &path)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        if (name.empty() || path.empty())
        {
            diagnostics.error("override set requires name and path");
            return 1;
        }
        auto resolved = std::filesystem::path(path);
        if (resolved.is_relative())
        {
            resolved = std::filesystem::current_path() / resolved;
        }
        std::error_code ec;
        resolved = std::filesystem::weakly_canonical(resolved, ec);
        if (ec)
        {
            resolved = std::filesystem::absolute(std::filesystem::path(path));
        }
        if (!File::dir(resolved))
        {
            diagnostics.error("dependency override path is not a directory for " + name + ": " + resolved.string());
            return 1;
        }

        LocalDependencyOverrides overrides;
        if (!DependencyOverrides::read(workspaceRoot, overrides, diagnostics))
        {
            return 1;
        }
        overrides.paths[name] = resolved.string();
        if (!DependencyOverrides::write(workspaceRoot, overrides, diagnostics))
        {
            return 1;
        }
        diagnostics.info("set dependency override " + name + " -> " + resolved.string());
        return 0;
    }

    int Commands::overrideRemove(const std::filesystem::path &workspaceRoot, const std::string &name)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        LocalDependencyOverrides overrides;
        if (!DependencyOverrides::read(workspaceRoot, overrides, diagnostics))
        {
            return 1;
        }
        const auto removed = overrides.paths.erase(name);
        if (!DependencyOverrides::write(workspaceRoot, overrides, diagnostics))
        {
            return 1;
        }
        diagnostics.info(removed == 0 ? "dependency override was not set: " + name
                                      : "removed dependency override " + name);
        return 0;
    }

    int Commands::overrideClear(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        LocalDependencyOverrides overrides;
        if (!DependencyOverrides::write(workspaceRoot, overrides, diagnostics))
        {
            return 1;
        }
        diagnostics.info("cleared dependency overrides");
        return 0;
    }

} // namespace rhodium
