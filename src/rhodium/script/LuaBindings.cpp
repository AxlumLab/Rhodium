/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/script/LuaBindings.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/script/LuaCondition.hpp"
#include "rhodium/script/LuaDependencyReader.hpp"
#include "rhodium/script/LuaDslDiagnostics.hpp"
#include "rhodium/script/LuaValueReader.hpp"

#include <sol/sol.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace rhodium
{
    namespace
    {
        struct DslContext
        {
            BuildModel &model;
            DiagnosticSink &diagnostics;
            std::string activePlatform;
            std::filesystem::path workspaceRoot;
            std::vector<std::filesystem::path> includeStack;
        };

        std::filesystem::path normalizePath(const std::filesystem::path &path)
        {
            std::error_code ec;
            auto absolute = std::filesystem::absolute(path, ec);
            if (ec)
            {
                absolute = path;
            }
            return absolute.lexically_normal();
        }

        bool isPathInside(const std::filesystem::path &path, const std::filesystem::path &root)
        {
            const auto normalizedPath = normalizePath(path);
            const auto normalizedRoot = normalizePath(root);
            const auto pathText = StringUtils::toLower(normalizedPath.string());
            auto rootText = StringUtils::toLower(normalizedRoot.string());
            if (!rootText.empty() && rootText.back() != '\\' && rootText.back() != '/')
            {
                rootText.push_back(std::filesystem::path::preferred_separator);
            }
            return pathText == StringUtils::toLower(normalizedRoot.string()) ||
                   pathText.rfind(rootText, 0) == 0;
        }

        std::filesystem::path resolveIncludePath(const DslContext &ctx, const std::string &value)
        {
            auto path = std::filesystem::path(value);
            if (path.is_absolute())
            {
                return normalizePath(path);
            }
            const auto base = ctx.includeStack.empty() ? ctx.workspaceRoot : ctx.includeStack.back().parent_path();
            return normalizePath(base / path);
        }

        void bindForbiddenProjectDsl(sol::state &lua, const std::shared_ptr<DslContext> &ctx)
        {
            const auto forbidden = {
                "module",
                "target",
                "sources",
                "public_deps",
                "private_deps",
                "custom_command",
                "pre_build",
                "post_build",
            };
            for (const auto *name : forbidden)
            {
                lua.set_function(name, [ctx, name](sol::variadic_args)
                                 { reportUnsupportedProjectDsl(ctx->diagnostics, name); });
            }
        }
    }

    void bindDsl(sol::state &lua,
                 BuildModel &model,
                 DiagnosticSink &diagnostics,
                 const std::string &activePlatform,
                 const std::filesystem::path &workspaceRoot,
                 const std::filesystem::path &manifestPath)
    {
        auto ctx = std::make_shared<DslContext>(DslContext{model, diagnostics, activePlatform});
        ctx->workspaceRoot = workspaceRoot.empty() ? std::filesystem::current_path() : normalizePath(workspaceRoot);
        if (!manifestPath.empty())
        {
            ctx->includeStack.push_back(normalizePath(manifestPath));
        }

        bindForbiddenProjectDsl(lua, ctx);

        lua.set_function("package", [ctx](const std::string &name)
                         { ctx->model.rootPackage.name = name; });
        lua.set_function("version", [ctx](const std::string &value)
                         { ctx->model.rootPackage.version = value; });
        lua.set_function("default_config", [ctx](const std::string &value)
                         { ctx->model.workspace.defaultConfig = StringUtils::toLower(value); });
        lua.set_function("default_platform", [ctx](const std::string &value)
                         {
            const auto normalized = StringUtils::toLower(value);
            ctx->model.workspace.defaultPlatform = normalized == "host" ? Environment::hostPlatformName() : normalized;
            ctx->activePlatform = ctx->model.workspace.defaultPlatform; });
        lua.set_function("default_arch", [ctx](const std::string &value)
                         { ctx->model.workspace.defaultArch = StringUtils::toLower(value); });
        lua.set_function("cmake", [ctx](sol::table options)
                         { ctx->model.workspace.generateCMakeUserPresets = getBoolOr(options, "generate_user_presets", ctx->model.workspace.generateCMakeUserPresets); });
        lua.set_function("dependency_overrides", [ctx](sol::table overrides)
                         { readDependencyOverrides(overrides, ctx->model, ctx->diagnostics); });
        lua.set_function("require", [ctx](const std::string &idOrSource, sol::table options)
                         { ctx->model.rootPackage.dependencies.push_back(dependencyFromOptions(idOrSource, options, false, ctx->activePlatform, ctx->workspaceRoot)); });
        lua.set_function("require_local", [ctx](const std::string &path, sol::table options)
                         { ctx->model.rootPackage.dependencies.push_back(dependencyFromOptions(path, options, true, ctx->activePlatform, ctx->workspaceRoot)); });
        lua.set_function("when", [ctx](const std::string &condition, sol::function body)
                         {
            if (!isValidCondition(condition))
            {
                ctx->diagnostics.error("when condition is invalid: " + condition);
                return;
            }
            if (conditionMatches(condition, ctx->activePlatform))
            {
                body();
            } });
        lua.set_function("include", [ctx, &lua](const std::string &path)
                         {
            const auto includePath = resolveIncludePath(*ctx, path);
            if (!isPathInside(includePath, ctx->workspaceRoot))
            {
                ctx->diagnostics.error("include path escapes workspace: " + path);
                return;
            }
            if (!std::filesystem::is_regular_file(includePath))
            {
                ctx->diagnostics.error("included lua file is missing: " + includePath.string());
                return;
            }
            if (std::find(ctx->includeStack.begin(), ctx->includeStack.end(), includePath) != ctx->includeStack.end())
            {
                ctx->diagnostics.error("recursive include is not allowed: " + includePath.string());
                return;
            }
            ctx->includeStack.push_back(includePath);
            sol::protected_function_result execResult = lua.script_file(includePath.string());
            ctx->includeStack.pop_back();
            if (!execResult.valid())
            {
                sol::error err = execResult;
                ctx->diagnostics.error(err.what());
            } });
    }

} // namespace rhodium
