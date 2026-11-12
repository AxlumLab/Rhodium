/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/export.hpp"

#include <string>

namespace rhodium
{

    class DiagnosticSink;

    class RHODIUM_EXPORT LuaDslDiagnostics
    {
    public:
        static void unsupported(DiagnosticSink &diagnostics, const std::string &name);
    };

} // namespace rhodium
