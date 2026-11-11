/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/diagnostics/DiagnosticSink.hpp"

namespace rhodium
{

    DiagnosticSink::~DiagnosticSink() = default;

    void DiagnosticSink::info(const std::string &message)
    {
        report({DiagnosticLevel::Info, message});
    }

    void DiagnosticSink::warning(const std::string &message)
    {
        report({DiagnosticLevel::Warning, message});
    }

    void DiagnosticSink::error(const std::string &message)
    {
        report({DiagnosticLevel::Error, message});
    }

    void DiagnosticSink::success(const std::string &message)
    {
        report({DiagnosticLevel::Success, message});
    }

} // namespace rhodium
