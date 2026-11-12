/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */
#include "rhodium/package/CMakePackageBuilder.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <string>
#include <map>
#include <vector>

namespace rhodium
{

    bool CMakePackageBuilder::run(const std::string &cmake, const std::vector<std::string> &args,
                                  const std::map<std::string, std::string> &environment, DiagnosticSink &diagnostics)
    {
        auto result = Process::run(cmake.empty() ? "cmake" : cmake, args, environment);
        if (!result.output.empty())
        {
            diagnostics.info(result.output);
        }
        if (result.exitCode != 0)
        {
            diagnostics.error("cmake command failed with exit code " + std::to_string(result.exitCode));
            return false;
        }
        return true;
    }

    class CMakePackageBuilder::Stages final
    {
    public:
        static std::vector<std::string> buildArgs(const PackageBuildRequest &request, const BuildUtil::Context &context,
                                                  const std::string &target)
        {
            const auto &dependency = request.dependency;
            const auto &buildDir = request.buildDir;
            const auto &cmakeBuildType = context.config;

            std::vector<std::string> args = {"--build", buildDir.string()};
            if (!target.empty())
            {
                args.push_back("--target");
                args.push_back(target);
            }
            if (!cmakeBuildType.empty())
            {
                args.push_back("--config");
                args.push_back(cmakeBuildType);
            }
            if (!dependency.cmake.buildArgs.empty())
            {
                auto extra = BuildUtil::expand(dependency.cmake.buildArgs, context);
                args.insert(args.end(), extra.begin(), extra.end());
            }
            return args;
        }

        static bool build(const PackageBuildRequest &request, const BuildUtil::Context &context,
                          const std::vector<std::string> &configureArgs, DiagnosticSink &diagnostics)
        {
            const auto &dependency = request.dependency;

            if (!CMakePackageBuilder::run(context.cmake, configureArgs, context.environment, diagnostics))
            {
                return false;
            }
            if (!dependency.cmake.buildTargets.empty())
            {
                for (const auto &target : dependency.cmake.buildTargets)
                {
                    if (target.empty())
                    {
                        diagnostics.error("cmake build_targets contains an empty target for package: " +
                                          dependency.name);
                        return false;
                    }
                    if (!CMakePackageBuilder::run(context.cmake, Stages::buildArgs(request, context, target),
                                                  context.environment, diagnostics))
                    {
                        return false;
                    }
                }
            }
            else
            {
                const auto buildTarget = !dependency.cmake.installTarget.empty() && !dependency.cmake.install
                                             ? dependency.cmake.installTarget
                                             : std::string{};
                if (!CMakePackageBuilder::run(context.cmake, Stages::buildArgs(request, context, buildTarget),
                                              context.environment, diagnostics))
                {
                    return false;
                }
            }
            if (!BuildUtil::markBuilt(context, diagnostics))
            {
                return false;
            }

            return true;
        }

        static bool install(const PackageBuildRequest &request, const BuildUtil::Context &context,
                            DiagnosticSink &diagnostics)
        {
            const auto &dependency = request.dependency;
            const auto &buildDir = request.buildDir;
            const auto &artifactDir = request.artifactDir;
            const auto &cmakeBuildType = context.config;
            if (dependency.cmake.install)
            {
                if (!dependency.cmake.installTarget.empty())
                {
                    std::vector<std::string> installTargetArgs = {"--build", buildDir.string(), "--target",
                                                                  dependency.cmake.installTarget};
                    if (!cmakeBuildType.empty())
                    {
                        installTargetArgs.push_back("--config");
                        installTargetArgs.push_back(cmakeBuildType);
                    }
                    auto extra = BuildUtil::expand(dependency.cmake.installArgs, context);
                    installTargetArgs.insert(installTargetArgs.end(), extra.begin(), extra.end());
                    if (!CMakePackageBuilder::run(context.cmake, installTargetArgs, context.environment, diagnostics))
                    {
                        return false;
                    }
                }
                else
                {
                    std::vector<std::string> installArgs = {"--install", buildDir.string(), "--prefix",
                                                            artifactDir.string()};
                    if (!cmakeBuildType.empty())
                    {
                        installArgs.push_back("--config");
                        installArgs.push_back(cmakeBuildType);
                    }
                    auto extra = BuildUtil::expand(dependency.cmake.installArgs, context);
                    installArgs.insert(installArgs.end(), extra.begin(), extra.end());
                    if (!CMakePackageBuilder::run(context.cmake, installArgs, context.environment, diagnostics))
                    {
                        return false;
                    }
                }
            }

            return true;
        }
    };

    bool CMakePackageBuilder::build(const PackageBuildRequest &request, PackageArtifact &artifact,
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

        const auto cmakeBuildType = dependency.cmake.buildType.empty() ? (config == "release" ? "Release" : "Debug")
                                                                       : dependency.cmake.buildType;
        BuildUtil::Context context{
            sourceDir, buildDir, artifactDir, dependency.name, cmakeBuildType, {}, {},
        };
        context.workspaceRoot = workspaceRoot;
        if (!BuildUtil::toolchain(context, preferredToolchain, dependency.toolchainRequirements, &diagnostics))
        {
            return false;
        }

        const auto env = BuildUtil::expand(dependency.cmake.env, context);
        const auto configureArgs = BuildUtil::wrap(sourceDir, env, "cmake", BuildUtil::cmakeArgs(dependency, context));

        if (options.runBuild && !Stages::build(request, context, configureArgs, diagnostics))
        {
            return false;
        }

        if (!options.runExport)
        {
            diagnostics.info("cmake package build stage completed: " + dependency.name);
            return true;
        }

        if (!Stages::install(request, context, diagnostics))
        {
            return false;
        }
        artifact.name = dependency.name;
        artifact.commit = commit;
        artifact.artifactId = dependency.name + "/" + commit;
        artifact.root = artifactDir;
        if (!BuildUtil::materialize(context, dependency.artifacts, artifact.artifacts, diagnostics))
        {
            return false;
        }
        if (artifact.artifacts.includeDirs.empty())
        {
            artifact.artifacts.includeDirs.push_back("include");
        }
        diagnostics.info("cmake package built: " + dependency.name);
        return true;
    }

} // namespace rhodium
