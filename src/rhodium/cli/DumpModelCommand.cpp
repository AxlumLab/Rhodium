/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/cli/Commands.hpp"

#include "rhodium/config/JsonModelWriter.hpp"
#include "rhodium/core/Logger.hpp"

#include <filesystem>

namespace rhodium
{
    int Commands::dumpModel(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            return 1;
        }
        LogInfo("{}", JsonModel::write(model));
        return 0;
    }

} // namespace rhodium
