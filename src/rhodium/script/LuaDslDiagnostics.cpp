/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/script/LuaDslDiagnostics.hpp"

#include "rhodium/diagnostics/DiagnosticSink.hpp"

namespace rhodium
{

    void LuaDslDiagnostics::unsupported(DiagnosticSink &diagnostics, const std::string &name)
    {
        diagnostics.error(
            "project build DSL '" + name +
            "' is not supported in rhodium.lua or tpkg.lua; use require()/require_local() to declare dependencies");
    }

} // namespace rhodium
