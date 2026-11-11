/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/PackageBuilderUtil.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/ArtifactMaterializer.hpp"
#include "rhodium/package/ArtifactValidator.hpp"
#include "rhodium/package/BuildVariableExpander.hpp"
#include "rhodium/package/CMakeArgumentBuilder.hpp"
#include "rhodium/package/ToolchainContextResolver.hpp"

#include <utility>

namespace rhodium
{

    bool BuildUtil::toolchain(Context &context,
                              const std::string &preferredToolchain,
                              const ToolchainRequirements &requirements,
                              DiagnosticSink *diagnostics)
    {
        return BuildToolchain::populate(context, preferredToolchain, requirements, diagnostics);
    }

    std::string BuildUtil::expand(std::string value, const Context &context)
    {
        return BuildVar::expand(std::move(value), context);
    }

    std::vector<std::string> BuildUtil::expand(const std::vector<std::string> &values, const Context &context)
    {
        return BuildVar::expand(values, context);
    }

    std::map<std::string, std::string> BuildUtil::expand(const std::map<std::string, std::string> &values, const Context &context)
    {
        return BuildVar::expand(values, context);
    }

    std::map<std::string, std::string> BuildUtil::cmakeOptions(const DependencyDesc &dependency)
    {
        return CMakeArgs::options(dependency);
    }

    std::vector<std::string> BuildUtil::cmakeArgs(const DependencyDesc &dependency, const Context &context)
    {
        return CMakeArgs::configure(dependency, context);
    }

    std::vector<std::string> BuildUtil::wrap(const std::filesystem::path &workingDir,
                                             const std::map<std::string, std::string> &env,
                                             const std::string &executable,
                                             const std::vector<std::string> &args)
    {
        return CMakeArgs::wrap(workingDir, env, executable, args);
    }

    bool BuildUtil::materialize(const Context &context,
                                const DependencyArtifacts &requested,
                                DependencyArtifacts &exported,
                                DiagnosticSink &diagnostics)
    {
        return ArtifactFiles::materialize(context, requested, exported, diagnostics);
    }

    bool BuildUtil::markBuilt(const Context &context, DiagnosticSink &diagnostics)
    {
        std::string error;
        if (!File::write(context.buildDir / ".tpkg_build_complete", "built\n", &error))
        {
            diagnostics.error("failed to write package build stage marker: " + error);
            return false;
        }
        return true;
    }

    bool BuildUtil::validate(const Context &context,
                             const DependencyArtifacts &artifacts,
                             DiagnosticSink &diagnostics)
    {
        return ArtifactCheck::validate(context, artifacts, diagnostics);
    }

} // namespace rhodium
