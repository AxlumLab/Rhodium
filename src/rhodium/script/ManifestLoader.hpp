/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/BuildModel.hpp"

#include <filesystem>

namespace rhodium
{

    class DiagnosticSink;

    struct ManifestLoadResult
    {
        bool ok = false;
        BuildModel model;
    };

    class RHODIUM_EXPORT ManifestLoader
    {
    public:
        ManifestLoadResult load(const std::filesystem::path &manifestPath, DiagnosticSink &diagnostics);
    };

} // namespace rhodium
