/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"

namespace rhodium
{

    void JsonDiagnosticSink::report(const Diagnostic &diagnostic)
    {
        diagnostics_.push_back({
            {"level", toString(diagnostic.level)},
            {"message", diagnostic.message},
        });
    }

    const nlohmann::json &JsonDiagnosticSink::diagnostics() const
    {
        return diagnostics_;
    }

} // namespace rhodium
