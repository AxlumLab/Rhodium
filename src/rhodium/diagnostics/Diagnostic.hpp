/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

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

    struct RHODIUM_EXPORT Diagnostic
    {
        static const char *name(DiagnosticLevel level);

        DiagnosticLevel level = DiagnosticLevel::Info;
        std::string message;
    };

} // namespace rhodium
