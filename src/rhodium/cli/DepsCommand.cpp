/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/cli/Commands.hpp"

#include "rhodium/core/Logger.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{
    int Commands::deps(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            return 1;
        }

        if (model.rootPackage.dependencies.empty())
        {
            LogInfo("dependencies: none");
            return 0;
        }

        for (const auto &dependency : model.rootPackage.dependencies)
        {
            LogInfo("{} {} ref={} subdir={} build={}", dependency.sourceType,
                    dependency.name,
                    dependency.ref.empty() ? "-" : dependency.ref,
                    dependency.subdir,
                    dependency.buildType);
        }
        return 0;
    }


} // namespace rhodium
