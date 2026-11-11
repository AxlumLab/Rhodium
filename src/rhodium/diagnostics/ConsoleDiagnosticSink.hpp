/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/diagnostics/DiagnosticSink.hpp"

namespace rhodium
{
    class ConsoleDiagnosticSink final : public DiagnosticSink
    {
    public:
        void report(const Diagnostic &diagnostic) override;
    };

} // namespace rhodium
