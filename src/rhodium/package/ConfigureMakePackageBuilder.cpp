/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/ConfigureMakePackageBuilder.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/MakePackageBuilder.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <string>
#include <vector>

namespace rhodium
{

    bool ConfigureMakePackageBuilder::build(const PackageBuildRequest &request, PackageArtifact &artifact,
                                            DiagnosticSink &diagnostics) const
    {
        const auto &[dependency, sourceDir, buildDir, artifactDir, commit, workspaceRoot, preferredToolchain, config,
                     options] = request;
        std::string error;
        if (!File::mkdir(buildDir, &error) || !File::mkdir(artifactDir, &error))
        {
            diagnostics.error("failed to create package build/artifact directories: " + error);
            return false;
        }

        BuildUtil::Context context{sourceDir, buildDir, artifactDir, dependency.name, config, {}, {}};
        context.workspaceRoot = workspaceRoot;
        if (!BuildUtil::toolchain(context, preferredToolchain, dependency.toolchainRequirements, &diagnostics))
        {
            return false;
        }
        if (options.runBuild)
        {
            auto rawConfigureArgs = dependency.configure.args;
            if (rawConfigureArgs.empty())
            {
                rawConfigureArgs.push_back("--prefix=$(artifact)");
            }
            std::vector<std::string> configureArgs = BuildUtil::expand(rawConfigureArgs, context);
            const auto commandArgs =
                BuildUtil::wrap(sourceDir, BuildUtil::expand(dependency.configure.env, context),
                                BuildUtil::expand(dependency.configure.script, context), configureArgs);
            auto result =
                Process::run(context.cmake.empty() ? "cmake" : context.cmake, commandArgs, context.environment);
            if (!result.output.empty())
            {
                diagnostics.info(result.output);
            }
            if (result.exitCode != 0)
            {
                diagnostics.error("configure command failed with exit code " + std::to_string(result.exitCode));
                return false;
            }
        }

        MakePackageBuilder make;
        auto makeDependency = dependency;
        if (makeDependency.make.installTargets.empty())
        {
            makeDependency.make.installTargets.push_back("install");
        }
        return make.build(PackageBuildRequest{.dependency = makeDependency,
                                              .sourceDir = sourceDir,
                                              .buildDir = buildDir,
                                              .artifactDir = artifactDir,
                                              .commit = commit,
                                              .workspaceRoot = workspaceRoot,
                                              .preferredToolchain = preferredToolchain,
                                              .config = config,
                                              .options = options},
                          artifact, diagnostics);
    }

} // namespace rhodium
