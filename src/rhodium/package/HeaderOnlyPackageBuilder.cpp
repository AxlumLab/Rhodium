/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/HeaderOnlyPackageBuilder.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

namespace rhodium
{

    bool HeaderOnlyPackageBuilder::build(const PackageBuildRequest &request, PackageArtifact &artifact,
                                         DiagnosticSink &diagnostics) const
    {
        const auto &[dependency, sourceDir, buildDir, artifactDir, commit, workspaceRoot, preferredToolchain, config,
                     options] = request;
        (void)preferredToolchain;
        if (!options.runExport)
        {
            diagnostics.info("header_only package has no build stage: " + dependency.name);
            return true;
        }
        std::string error;
        if (!File::mkdir(artifactDir, &error))
        {
            diagnostics.error("failed to create artifact directory: " + error);
            return false;
        }

        artifact.name = dependency.name;
        artifact.commit = commit;
        artifact.artifactId = dependency.name + "/" + commit;
        artifact.root = artifactDir;
        auto requested = dependency.artifacts;
        requested.mode = "copy";
        if (requested.includeDirs.empty())
        {
            requested.includeDirs.push_back("include");
        }
        BuildUtil::Context context{sourceDir, artifactDir, artifactDir, dependency.name, config, {}, {}};
        context.workspaceRoot = workspaceRoot;
        context.toolchains = options.toolchains;
        if (!BuildUtil::materialize(context, requested, artifact.artifacts, diagnostics))
        {
            return false;
        }
        diagnostics.info("header_only package restored: " + dependency.name);
        return true;
    }

} // namespace rhodium
