/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include <string>

namespace rhodium
{

    enum class DiagnosticLevel
    {
        Info,
        Warning,
        Error,
        Success,
    };

    struct Diagnostic
    {
        DiagnosticLevel level = DiagnosticLevel::Info;
        std::string message;
    };

    const char *toString(DiagnosticLevel level);

} // namespace rhodium
