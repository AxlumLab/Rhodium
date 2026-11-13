/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <nlohmann/json.hpp>

namespace rhodium
{

    class RHODIUM_EXPORT JsonDiagnosticSink final : public DiagnosticSink
    {
    public:
        void report(const Diagnostic &diagnostic) override;
        const nlohmann::json &diagnostics() const;

    private:
        nlohmann::json diagnostics_ = nlohmann::json::array();
    };

} // namespace rhodium
