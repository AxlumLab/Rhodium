/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/script/ManifestLoader.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/Diagnostic.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/model/BuildModel.hpp"
#include "rhodium/script/LuaBindings.hpp"
#include "rhodium/script/LuaRuntime.hpp"

#include <sol/sol.hpp>

namespace rhodium
{
    class CountingDiagnosticSink final : public DiagnosticSink
    {
    public:
        explicit CountingDiagnosticSink(DiagnosticSink &inner) : m_inner(inner) {}

        void report(const Diagnostic &diagnostic) override
        {
            if (diagnostic.level == DiagnosticLevel::Error)
            {
                ++m_errorCount;
            }
            m_inner.report(diagnostic);
        }

        int errorCount() const
        {
            return m_errorCount;
        }

    private:
        DiagnosticSink &m_inner;
        int m_errorCount = 0;
    };

    ManifestLoadResult ManifestLoader::load(const std::filesystem::path &manifestPath, DiagnosticSink &diagnostics)
    {
        ManifestLoadResult result;
        result.model.manifestName = manifestPath.filename().string();
        CountingDiagnosticSink countedDiagnostics(diagnostics);
        if (!File::exists(manifestPath))
        {
            countedDiagnostics.error("missing dependency manifest (rhodium.lua or tpkg.lua): " + manifestPath.string());
            return result;
        }

        try
        {
            LuaRuntime runtime;
            auto &lua = runtime.state();
            const auto platform = result.model.workspace.defaultPlatform.empty()
                                      ? Environment::hostPlatformName()
                                      : result.model.workspace.defaultPlatform;
            LuaBindings::bind(lua, result.model, countedDiagnostics,
                              LuaBindingOptions{.activePlatform = platform,
                                                .workspaceRoot = manifestPath.parent_path(),
                                                .manifestPath = manifestPath});

            sol::protected_function_result execResult = lua.script_file(manifestPath.string());
            if (!execResult.valid())
            {
                sol::error err = execResult;
                countedDiagnostics.error(err.what());
                return result;
            }

            if (!ModelValidator::validate(result.model, countedDiagnostics) || countedDiagnostics.errorCount() > 0)
            {
                return result;
            }

            result.ok = true;
            return result;
        }
        catch (const std::exception &ex)
        {
            countedDiagnostics.error(ex.what());
            return result;
        }
    }

} // namespace rhodium
