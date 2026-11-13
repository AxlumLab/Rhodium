/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/diagnostics/DiagnosticSink.hpp"

namespace rhodium
{
    class RHODIUM_EXPORT ConsoleDiagnosticSink final : public DiagnosticSink
    {
    public:
        void report(const Diagnostic &diagnostic) override;
    };

} // namespace rhodium
