/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/diagnostics/Diagnostic.hpp"

namespace rhodium
{
    const char *toString(DiagnosticLevel level)
    {
        switch (level)
        {
        case DiagnosticLevel::Info:
            return "info";
        case DiagnosticLevel::Warning:
            return "warn";
        case DiagnosticLevel::Error:
            return "error";
        case DiagnosticLevel::Success:
            return "success";
        }
        return "unknown";
    }

} // namespace rhodium
