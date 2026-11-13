/*
 * @Author: khfahqp khfahqp@gmail.com
 * @LastEditors: khfahqp khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/CleanSession.hpp"
#include "rhodium/core/WorkspaceLock.hpp"
#include "rhodium/cli/Commands.hpp"

#include "rhodium/diagnostics/ConsoleDiagnosticSink.hpp"

#include <filesystem>

namespace rhodium
{
    int Commands::clean(const std::filesystem::path &workspaceRoot, const CleanOptions &options)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        return CleanSession(workspaceRoot, options, diagnostics).run();
    }

} // namespace rhodium
