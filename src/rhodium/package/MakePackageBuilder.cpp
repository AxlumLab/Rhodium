/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/MakePackageBuilder.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <string>
#include <vector>

namespace rhodium
{

    bool MakePackageBuilder::run(const std::filesystem::path &workingDir,
                                 const MakeBuildConfig &make,
                                 const BuildUtil::Context &context,
                                 const std::vector<std::string> &extraArgs,
                                 const std::vector<std::string> &targets,
                                 DiagnosticSink &diagnostics)
    {
        std::vector<std::string> args;
        if (make.jobs > 0)
        {
            args.push_back("-j" + std::to_string(make.jobs));
        }
        auto expandedExtra = BuildUtil::expand(extraArgs, context);
        args.insert(args.end(), expandedExtra.begin(), expandedExtra.end());
        auto expandedTargets = BuildUtil::expand(targets, context);
        args.insert(args.end(), expandedTargets.begin(), expandedTargets.end());

        const auto commandArgs = BuildUtil::wrap(
            workingDir,
            BuildUtil::expand(make.env, context),
            make.executable.empty() ? "make" : make.executable,
            args);
        auto result = Process::run(context.cmake.empty() ? "cmake" : context.cmake, commandArgs, context.environment);
        if (!result.output.empty())
        {
            diagnostics.info(result.output);
        }
        if (result.exitCode != 0)
        {
            diagnostics.error("make command failed with exit code " + std::to_string(result.exitCode));
            return false;
        }
        return true;
    }

    bool MakePackageBuilder::build(const DependencyDesc &dependency,
                                   const std::filesystem::path &sourceDir,
                                   const std::filesystem::path &buildDir,
                                   const std::filesystem::path &artifactDir,
                                   const std::string &commit,
                                   const std::filesystem::path &workspaceRoot,
                                   const std::string &preferredToolchain,
                                   const std::string &config,
                                   const PackageBuildOptions &options,
                                   PackageArtifact &artifact,
                                   DiagnosticSink &diagnostics) const
    {
        std::string error;
        if (!File::mkdir(buildDir, &error) || !File::mkdir(artifactDir, &error))
        {
            diagnostics.error("failed to create package build/artifact directories: " + error);
            return false;
        }

        BuildUtil::Context context{sourceDir, buildDir, artifactDir, dependency.name, config, {}, {}};
        context.workspaceRoot = workspaceRoot;
        context.toolchains = options.toolchains;
        if (!BuildUtil::toolchain(context, preferredToolchain, dependency.toolchainRequirements, &diagnostics))
        {
            return false;
        }
        if (options.runBuild)
        {
            if (!run(sourceDir, dependency.make, context, dependency.make.args, dependency.make.targets, diagnostics))
            {
                return false;
            }
            if (!BuildUtil::markBuilt(context, diagnostics))
            {
                return false;
            }
        }
        if (!options.runExport)
        {
            return true;
        }
        if (!dependency.make.installTargets.empty())
        {
            if (!run(sourceDir, dependency.make, context, dependency.make.installArgs, dependency.make.installTargets, diagnostics))
            {
                return false;
            }
        }

        artifact.name = dependency.name;
        artifact.commit = commit;
        artifact.artifactId = dependency.name + "/" + commit;
        artifact.root = artifactDir;
        if (!BuildUtil::materialize(context, dependency.artifacts, artifact.artifacts, diagnostics))
        {
            return false;
        }
        return true;
    }

} // namespace rhodium
