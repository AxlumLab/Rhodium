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
#include <utility>

namespace rhodium
{

    struct DslContext
    {
        BuildModel &model;
        DiagnosticSink &diagnostics;
        std::string activePlatform;
        std::filesystem::path workspaceRoot;
        std::vector<std::filesystem::path> includeStack;
    };

    class LuaIncludes final
    {
    public:
        static std::filesystem::path normalize(const std::filesystem::path &path)
        {
            std::error_code ec;
            auto absolute = std::filesystem::absolute(path, ec);
            if (ec)
            {
                absolute = path;
            }
            return absolute.lexically_normal();
        }

        static bool contains(const std::filesystem::path &path, const std::filesystem::path &root)
        {
            const auto normalizedPath = LuaIncludes::normalize(path);
            const auto normalizedRoot = LuaIncludes::normalize(root);
            const auto pathText = StringUtils::toLower(normalizedPath.string());
            auto rootText = StringUtils::toLower(normalizedRoot.string());
            if (!rootText.empty() && rootText.back() != '\\' && rootText.back() != '/')
            {
                rootText.push_back(std::filesystem::path::preferred_separator);
            }
            return pathText == StringUtils::toLower(normalizedRoot.string()) || pathText.rfind(rootText, 0) == 0;
        }

        static std::filesystem::path resolve(const DslContext &ctx, const std::string &value)
        {
            auto path = std::filesystem::path(value);
            if (path.is_absolute())
            {
                return LuaIncludes::normalize(path);
            }
            const auto base = ctx.includeStack.empty() ? ctx.workspaceRoot : ctx.includeStack.back().parent_path();
            return LuaIncludes::normalize(base / path);
        }

        static void bindForbidden(sol::state &lua, const std::shared_ptr<DslContext> &ctx)
        {
            const auto forbidden = {
                "module",       "target",         "sources",   "public_deps",
                "private_deps", "custom_command", "pre_build", "post_build",
            };
            for (const auto *name : forbidden)
            {
                lua.set_function(name, [ctx, name](sol::variadic_args)
                                 { LuaDslDiagnostics::unsupported(ctx->diagnostics, name); });
            }
        }
    };

    class DslRegistration final
    {
    public:
        DslRegistration(sol::state &lua, std::shared_ptr<DslContext> context) : lua_(lua), context_(std::move(context))
        {
        }

        void bind()
        {
            LuaIncludes::bindForbidden(lua_, context_);
            metadata();
            dependencies();
            condition();
            includes();
        }

    private:
        void metadata()
        {
            auto &lua = lua_;
            const auto ctx = context_;
            lua.set_function("package", [ctx](const std::string &name) { ctx->model.rootPackage.name = name; });
            lua.set_function("version", [ctx](const std::string &value) { ctx->model.rootPackage.version = value; });
            lua.set_function("default_config", [ctx](const std::string &value)
                             { ctx->model.workspace.defaultConfig = StringUtils::toLower(value); });
            lua.set_function("default_platform",
                             [ctx](const std::string &value)
                             {
                                 const auto normalized = StringUtils::toLower(value);
                                 ctx->model.workspace.defaultPlatform =
                                     normalized == "host" ? Environment::hostPlatformName() : normalized;
                                 ctx->activePlatform = ctx->model.workspace.defaultPlatform;
                             });
            lua.set_function("default_arch", [ctx](const std::string &value)
                             { ctx->model.workspace.defaultArch = StringUtils::toLower(value); });
            lua.set_function("cmake",
                             [ctx](sol::table options)
                             {
                                 ctx->model.workspace.generateCMakeUserPresets = LuaValueReader::boolean(
                                     options, "generate_user_presets", ctx->model.workspace.generateCMakeUserPresets);
                             });
        }

        void dependencies()
        {
            auto &lua = lua_;
            const auto ctx = context_;
            lua.set_function("dependency_overrides", [ctx](sol::table overrides)
                             { LuaDependencyReader::overrides(overrides, ctx->model, ctx->diagnostics); });
            lua.set_function(
                "require",
                [ctx](const std::string &idOrSource, sol::table options)
                {
                    ctx->model.rootPackage.dependencies.push_back(
                        LuaDependencyReader(ctx->activePlatform, ctx->workspaceRoot).read(idOrSource, options, false));
                });
            lua.set_function(
                "require_local",
                [ctx](const std::string &path, sol::table options)
                {
                    ctx->model.rootPackage.dependencies.push_back(
                        LuaDependencyReader(ctx->activePlatform, ctx->workspaceRoot).read(path, options, true));
                });
        }

        void condition()
        {
            auto &lua = lua_;
            const auto ctx = context_;
            lua.set_function("when",
                             [ctx](const std::string &condition, sol::function body)
                             {
                                 if (!LuaCondition::valid(condition))
                                 {
                                     ctx->diagnostics.error("when condition is invalid: " + condition);
                                     return;
                                 }
                                 if (LuaCondition::matches(condition, ctx->activePlatform))
                                 {
                                     body();
                                 }
                             });
        }

        void includes()
        {
            auto &lua = lua_;
            const auto ctx = context_;
            lua.set_function("include",
                             [ctx, &lua](const std::string &path)
                             {
                                 const auto includePath = LuaIncludes::resolve(*ctx, path);
                                 if (!LuaIncludes::contains(includePath, ctx->workspaceRoot))
                                 {
                                     ctx->diagnostics.error("include path escapes workspace: " + path);
                                     return;
                                 }
                                 if (!std::filesystem::is_regular_file(includePath))
                                 {
                                     ctx->diagnostics.error("included lua file is missing: " + includePath.string());
                                     return;
                                 }
                                 if (std::find(ctx->includeStack.begin(), ctx->includeStack.end(), includePath) !=
                                     ctx->includeStack.end())
                                 {
                                     ctx->diagnostics.error("recursive include is not allowed: " +
                                                            includePath.string());
                                     return;
                                 }
                                 ctx->includeStack.push_back(includePath);
                                 sol::protected_function_result execResult = lua.script_file(includePath.string());
                                 ctx->includeStack.pop_back();
                                 if (!execResult.valid())
                                 {
                                     sol::error err = execResult;
                                     ctx->diagnostics.error(err.what());
                                 }
                             });
        }

        sol::state &lua_;
        std::shared_ptr<DslContext> context_;
    };

    void LuaBindings::bind(sol::state &lua, BuildModel &model, DiagnosticSink &diagnostics,
                           const LuaBindingOptions &options)
    {
        const auto &[activePlatform, workspaceRoot, manifestPath] = options;

        auto ctx = std::make_shared<DslContext>(DslContext{model, diagnostics, activePlatform});
        ctx->workspaceRoot =
            workspaceRoot.empty() ? std::filesystem::current_path() : LuaIncludes::normalize(workspaceRoot);
        if (!manifestPath.empty())
        {
            ctx->includeStack.push_back(LuaIncludes::normalize(manifestPath));
        }

        DslRegistration(lua, ctx).bind();
    }

} // namespace rhodium
