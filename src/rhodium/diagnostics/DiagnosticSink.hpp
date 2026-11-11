/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/diagnostics/Diagnostic.hpp"

#include <string>

namespace rhodium
{
    class DiagnosticSink
    {
    public:
        virtual ~DiagnosticSink();

        virtual void report(const Diagnostic &diagnostic) = 0;

        void info(const std::string &message);
        void warning(const std::string &message);
        void error(const std::string &message);
        void success(const std::string &message);
    };

} // namespace rhodium
