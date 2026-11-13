/*
 * @Author: khfahqp khfahqp@gmail.com
 * @LastEditors: khfahqp khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include <filesystem>
#include <string>

namespace rhodium
{

    struct CleanOptions
    {
        bool all = false;
        bool artifacts = false;
        bool sources = false;
        bool lock = false;
        std::string packageName;
    };

    class RHODIUM_EXPORT CleanSession final
    {
    public:
        CleanSession(const std::filesystem::path &workspaceRoot, const CleanOptions &options, DiagnosticSink &diagnostics);

        int run();

    private:
        bool resolveRoot();
        bool allowed(const std::filesystem::path &path) const;
        bool remove(const std::filesystem::path &path);
        bool cleanCaches();
        bool cleanLock();

    private:
        const std::filesystem::path &m_workspaceRoot;
        const CleanOptions &m_options;
        DiagnosticSink &m_diagnostics;
        const std::filesystem::path m_rhodiumRoot;
        std::filesystem::path m_normalizedRHODIUMRoot;
    };

}
