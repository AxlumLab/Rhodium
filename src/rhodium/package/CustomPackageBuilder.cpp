/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/CustomPackageBuilder.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace rhodium
{

    std::string CustomPackageBuilder::listing(const std::filesystem::path &path)
    {
        if (!std::filesystem::is_directory(path))
        {
            return path.string() + " <missing>";
        }

        std::ostringstream listing;
        listing << path.string() << ":";
        bool any = false;
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            any = true;
            listing << "\n  " << entry.path().filename().string();
            if (entry.is_regular_file())
            {
                listing << " (" << entry.file_size() << " bytes)";
            }
        }
        if (!any)
        {
            listing << "\n  <empty>";
        }
        return listing.str();
    }

    bool CustomPackageBuilder::run(const std::vector<std::string> &commands,
                                   const std::string &phase,
                                   const BuildUtil::Context &context,
                                   DiagnosticSink &diagnostics)
    {
        for (std::size_t index = 0; index < commands.size(); ++index)
        {
            const auto expanded = BuildUtil::expand(commands[index], context);
            diagnostics.info("custom package " + phase + ": " + expanded);

#ifdef _WIN32
            const auto scriptPath = context.buildDir / ("rhodium-custom-" + phase + "-" + std::to_string(index) + ".cmd");
            const std::string script = "@echo off\r\n" + expanded + "\r\n";
#else
            const auto scriptPath = context.buildDir / ("rhodium-custom-" + phase + "-" + std::to_string(index) + ".sh");
            const std::string script = "#!/bin/sh\nset -e\n" + expanded + "\n";
#endif
            std::string error;
            if (!File::write(scriptPath, script, &error))
            {
                diagnostics.error("failed to write custom package script: " + error);
                return false;
            }

#ifdef _WIN32
            const auto result = Process::runShell(scriptPath.string(), ProcessShell::Cmd, context.environment);
#else
            const auto result = Process::run("sh", {scriptPath.string()}, context.environment);
#endif
            if (!result.output.empty())
            {
                diagnostics.info(result.output);
            }
            if (result.exitCode != 0)
            {
                diagnostics.error("custom package command failed with exit code " + std::to_string(result.exitCode));
                return false;
            }
        }
        return true;
    }

    bool CustomPackageBuilder::build(const DependencyDesc &dependency,
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
            if (!run(dependency.commands.configure, "configure", context, diagnostics) ||
                !run(dependency.commands.build, "build", context, diagnostics))
            {
                return false;
            }
            if (!BuildUtil::markBuilt(context, diagnostics))
            {
                return false;
            }
        }
        diagnostics.info("custom package artifact lib after build: " + listing(artifactDir / "lib"));
        if (!options.runExport)
        {
            return true;
        }
        if (!run(dependency.commands.install, "install", context, diagnostics))
        {
            return false;
        }
        diagnostics.info("custom package artifact include after install: " + listing(artifactDir / "include"));
        diagnostics.info("custom package artifact lib after install: " + listing(artifactDir / "lib"));

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
