/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/cli/Commands.hpp"

#include <filesystem>

namespace rhodium
{
    int Commands::load(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            return 1;
        }
        diagnostics.info("loaded " + CliWorkspace::manifest(workspaceRoot).string());
        return 0;
    }

} // namespace rhodium
