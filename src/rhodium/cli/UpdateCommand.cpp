/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/cli/Commands.hpp"

#include "rhodium/diagnostics/ConsoleDiagnosticSink.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    int Commands::update(const std::filesystem::path &workspaceRoot,
                      const std::string &packageName,
                      bool updateAll,
                      const std::string &preferredToolchain,
                      const std::string &configOption,
                      const std::vector<std::string> &cliOverrides)
    {
        if (!updateAll && packageName.empty())
        {
            ConsoleDiagnosticSink diagnostics;
            diagnostics.error("update requires a package name or --all");
            return 1;
        }
        return restore(workspaceRoot, preferredToolchain, configOption, cliOverrides, false, {}, false, false, false, packageName, updateAll);
    }

} // namespace rhodium
