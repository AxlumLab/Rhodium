/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/cli/Commands.hpp"

#include "rhodium/diagnostics/ConsoleDiagnosticSink.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    int Commands::update(const std::filesystem::path &workspaceRoot, const UpdateRequest &request)
    {
        const auto &[packageName, updateAll, preferredToolchain, configOption, cliOverrides] = request;
        if (!updateAll && packageName.empty())
        {
            ConsoleDiagnosticSink diagnostics;
            diagnostics.error("update requires a package name or --all");
            return 1;
        }
        return restore(workspaceRoot, RestoreRequest{.preferredToolchain = preferredToolchain,
                                                     .configOption = configOption,
                                                     .cliOverrides = cliOverrides,
                                                     .locked = false,
                                                     .packageName = {},
                                                     .buildOnly = false,
                                                     .exportOnly = false,
                                                     .rebuild = false,
                                                     .updatePackage = packageName,
                                                     .updateAll = updateAll});
    }

} // namespace rhodium
