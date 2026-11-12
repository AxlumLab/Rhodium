#include "rhodium/core/Path.hpp"

#include "rhodium/resolve/DependencyBuildStep.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/CMakePackageBuilder.hpp"
#include "rhodium/package/ConfigureMakePackageBuilder.hpp"
#include "rhodium/package/CustomPackageBuilder.hpp"
#include "rhodium/package/HeaderOnlyPackageBuilder.hpp"
#include "rhodium/package/MakePackageBuilder.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/package/PrebuiltPackageBuilder.hpp"
#include "rhodium/resolve/DependencyExportStep.hpp"
#include "rhodium/resolve/DependencyLockStep.hpp"

#include <sstream>
#include <system_error>

namespace rhodium
{

    bool DepBuild::run(const Request &request, const Context &context, Result &result, DiagnosticSink &diagnostics)
    {
        const auto &[dependency, sourceRoot, commit, locked, wantsUpdate] = request;
        result = {};
        result.updated = LockedPackage::from(dependency, commit, context.effectiveConfig, context.preferredToolchain,
                                             context.options.toolchainContentHash);
        result.updated.source = DepLock::source(context.workspaceRoot, dependency);

        const auto buildKey = key(context.effectiveConfig, context.preferredToolchain);
        const auto buildDir = context.cache.build(dependency, buildKey);
        result.artifactDir = Path::stateRoot(context.workspaceRoot) / "artifacts" / result.updated.artifactId;
        const bool artifactReusable = !dependency.local && locked && !wantsUpdate &&
                                      DepExport::reusable(*locked, result.updated, result.artifactDir);
        if (artifactReusable && !context.options.buildOnly && !context.options.rebuild)
        {
            diagnostics.info("package artifact is up to date; skipping build: " + dependency.name);
            if (!DepExport::gc(context.lockFile, context.workspaceRoot,
                               BuildSelection{context.effectiveConfig, context.preferredToolchain}, dependency.name,
                               diagnostics))
            {
                return false;
            }
            result.skipPackage = true;
            return true;
        }
        if (context.options.rebuild && !removeBeforeRebuild(buildDir, diagnostics))
        {
            return false;
        }
        removeSiblings(context.cache, dependency, buildKey, retained(context.lockFile, dependency.name, buildKey),
                       diagnostics);
        const bool hasBuildStage = hasStage(dependency);
        bool buildDirExists = File::dir(buildDir);
        if (buildDirExists && hasCMakeCache(buildDir) && !cacheMatchesDir(buildDir))
        {
            diagnostics.warning("cmake build cache was moved from another path; removing it before rebuild: " +
                                buildDir.string());
            if (!removeBeforeRebuild(buildDir, diagnostics))
            {
                return false;
            }
            buildDirExists = false;
        }
        PackageBuildOptions buildOptions;
        const bool completedBuildStage = buildDirExists && completed(buildDir);
        buildOptions.runBuild = hasBuildStage && !context.options.exportOnly &&
                                (context.options.buildOnly || context.options.rebuild || !completedBuildStage);
        buildOptions.runExport = !context.options.buildOnly;

        if (context.options.exportOnly && hasBuildStage && !completedBuildStage)
        {
            diagnostics.error("restore --export-only requires a completed build directory for package " +
                              dependency.name + ": " + buildDir.string());
            return false;
        }

        auto packageBuilder = builder(dependency);
        if (!packageBuilder->build(PackageBuildRequest{.dependency = dependency,
                                                       .sourceDir = subdir(sourceRoot, dependency),
                                                       .buildDir = buildDir,
                                                       .artifactDir = result.artifactDir,
                                                       .commit = commit,
                                                       .workspaceRoot = context.workspaceRoot,
                                                       .preferredToolchain = context.preferredToolchain,
                                                       .config = context.effectiveConfig,
                                                       .options = buildOptions},
                                   result.artifact, diagnostics))
        {
            reportFailure(dependency, sourceRoot, buildDir, diagnostics);
            return false;
        }

        return true;
    }

    void DepBuild::reportFailure(const DependencyDesc &dependency, const std::filesystem::path &sourceRoot,
                                 const std::filesystem::path &buildDir, DiagnosticSink &diagnostics)
    {
        diagnostics.error("failed to build package: " + dependency.name);
        diagnostics.info("build directory: " + buildDir.string());
        diagnostics.info("source directory: " + subdir(sourceRoot, dependency).string());

        if (dependency.sourceType == "git" && !dependency.ref.empty())
        {
            diagnostics.info("specified ref: " + dependency.ref);
            diagnostics.info("tip: check if the ref exists in the repository");
            diagnostics.info("tip: run 'git tag' or 'git branch -r' in the source directory to see available versions");
        }
    }

    std::unique_ptr<PackageBuilder> DepBuild::builder(const DependencyDesc &dependency)
    {
        if (dependency.buildType == "cmake")
        {
            return std::make_unique<CMakePackageBuilder>();
        }
        if (dependency.buildType == "prebuilt")
        {
            return std::make_unique<PrebuiltPackageBuilder>();
        }
        if (dependency.buildType == "make")
        {
            return std::make_unique<MakePackageBuilder>();
        }
        if (dependency.buildType == "configure_make")
        {
            return std::make_unique<ConfigureMakePackageBuilder>();
        }
        if (dependency.buildType == "custom")
        {
            return std::make_unique<CustomPackageBuilder>();
        }
        return std::make_unique<HeaderOnlyPackageBuilder>();
    }

    void DepBuild::applyConfig(DependencyDesc &dependency, const std::string &config)
    {
        const auto normalized = BuildConfig::normalize(config);
        const auto exact = dependency.artifacts.libFilesByConfig.find(normalized);
        if (exact != dependency.artifacts.libFilesByConfig.end())
        {
            dependency.artifacts.libFiles = exact->second;
            dependency.artifacts.libFilesByConfig.clear();
            return;
        }
        const auto all = dependency.artifacts.libFilesByConfig.find("all");
        if (all != dependency.artifacts.libFilesByConfig.end())
        {
            dependency.artifacts.libFiles = all->second;
            dependency.artifacts.libFilesByConfig.clear();
            return;
        }
        if (!dependency.artifacts.libFilesByConfig.empty())
        {
            dependency.artifacts.libFiles.clear();
            dependency.artifacts.libFilesByConfig.clear();
        }
    }

    std::filesystem::path DepBuild::subdir(const std::filesystem::path &source, const DependencyDesc &dependency)
    {
        if (dependency.subdir.empty() || dependency.subdir == ".")
        {
            return source;
        }
        return source / dependency.subdir;
    }

    std::string DepBuild::key(const std::string &config, const std::string &toolchainId)
    {
        return BuildConfig::normalize(config) + "-" + (toolchainId.empty() ? std::string("default") : toolchainId);
    }

    std::set<std::string> DepBuild::retained(const LockFile &lockFile, const std::string &packageName,
                                             const std::string &currentPkgKey)
    {
        std::set<std::string> keys{currentPkgKey};
        for (const auto &package : lockFile.packages)
        {
            if (package.name != packageName)
            {
                continue;
            }
            keys.insert(key(package.config, package.toolchainId));
        }
        return keys;
    }

    void DepBuild::removeSiblings(const PackageCache &cache, const DependencyDesc &dependency,
                                  const std::string &currentPkgKey, const std::set<std::string> &keepKeys,
                                  DiagnosticSink &diagnostics)
    {
        const auto buildRoot = cache.build(dependency, currentPkgKey).parent_path();
        std::error_code ec;
        if (!std::filesystem::is_directory(buildRoot, ec))
        {
            return;
        }

        for (const auto &entry : std::filesystem::directory_iterator(buildRoot, ec))
        {
            if (ec)
            {
                diagnostics.warning("failed to scan package build cache: " + ec.message());
                return;
            }
            if (!entry.is_directory(ec))
            {
                continue;
            }
            const auto name = entry.path().filename().string();
            if (keepKeys.find(name) != keepKeys.end())
            {
                continue;
            }

            std::filesystem::remove_all(entry.path(), ec);
            if (ec)
            {
                diagnostics.warning("failed to remove stale package build cache " + entry.path().string() + ": " +
                                    ec.message());
                ec.clear();
            }
            else
            {
                diagnostics.info("removed stale package build cache: " + entry.path().string());
            }
        }
    }

    bool DepBuild::removeBeforeRebuild(const std::filesystem::path &buildDir, DiagnosticSink &diagnostics)
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(buildDir, ec))
        {
            return true;
        }

        std::filesystem::remove_all(buildDir, ec);
        if (ec)
        {
            diagnostics.error("failed to remove stale package build cache " + buildDir.string() + ": " + ec.message());
            return false;
        }
        diagnostics.info("removed stale package build cache before rebuild: " + buildDir.string());
        return true;
    }

    std::string DepBuild::normalized(const std::filesystem::path &path)
    {
        auto text = path.lexically_normal().generic_string();
        return StringUtils::toLower(text);
    }

    bool DepBuild::hasCMakeCache(const std::filesystem::path &buildDir)
    {
        return File::exists(buildDir / "CMakeCache.txt");
    }

    bool DepBuild::cacheMatchesDir(const std::filesystem::path &buildDir)
    {
        const auto cachePath = buildDir / "CMakeCache.txt";
        if (!hasCMakeCache(buildDir))
        {
            return true;
        }

        const auto expected = normalized(buildDir);
        std::istringstream lines(File::read(cachePath));
        std::string line;
        while (std::getline(lines, line))
        {
            if (line.rfind("CMAKE_CACHEFILE_DIR:INTERNAL=", 0) == 0)
            {
                const std::string prefix = "CMAKE_CACHEFILE_DIR:INTERNAL=";
                const auto actual = StringUtils::toLower(
                    std::filesystem::path(line.substr(prefix.size())).lexically_normal().generic_string());
                return actual == expected;
            }
        }
        return true;
    }

    bool DepBuild::hasStage(const DependencyDesc &dependency)
    {
        return dependency.buildType == "cmake" || dependency.buildType == "make" ||
               dependency.buildType == "configure_make" || dependency.buildType == "custom";
    }

    bool DepBuild::completed(const std::filesystem::path &buildDir)
    {
        return File::exists(buildDir / ".tpkg_build_complete");
    }

} // namespace rhodium
