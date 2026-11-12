/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/PrebuiltPackageBuilder.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <vector>

namespace rhodium
{

    bool PrebuiltPackageBuilder::dirs(const std::filesystem::path &root, const std::vector<std::string> &dirs,
                                      const std::string &field, DiagnosticSink &diagnostics)
    {
        for (const auto &dir : dirs)
        {
            const auto path = root / dir;
            if (!File::dir(path))
            {
                diagnostics.error("prebuilt package " + field + " path is missing: " + path.string());
                return false;
            }
        }
        return true;
    }

    bool PrebuiltPackageBuilder::build(const PackageBuildRequest &request, PackageArtifact &artifact,
                                       DiagnosticSink &diagnostics) const
    {
        const auto &[dependency, sourceDir, buildDir, artifactDir, commit, workspaceRoot, preferredToolchain, config,
                     options] = request;
        (void)preferredToolchain;
        if (!options.runExport)
        {
            diagnostics.info("prebuilt package has no build stage: " + dependency.name);
            return true;
        }
        if (!dirs(sourceDir, dependency.artifacts.includeDirs, "include_dirs", diagnostics) ||
            !dirs(sourceDir, dependency.artifacts.libDirs, "lib_dirs", diagnostics) ||
            !dirs(sourceDir, dependency.artifacts.binDirs, "bin_dirs", diagnostics))
        {
            return false;
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
        BuildUtil::Context context{sourceDir, artifactDir, artifactDir, dependency.name, config, {}, {}};
        context.workspaceRoot = workspaceRoot;
        context.toolchains = options.toolchains;
        if (!BuildUtil::materialize(context, requested, artifact.artifacts, diagnostics))
        {
            return false;
        }
        diagnostics.info("prebuilt package validated: " + dependency.name);
        return true;
    }

} // namespace rhodium
