#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/cli/Commands.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Logger.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/ArtifactGc.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/package/PackageArtifactStore.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class NullDiagnosticSink final : public DiagnosticSink
    {
    public:
        void report(const Diagnostic &) override {}
    };

    class PackageStatus final
    {
    public:
        static std::string read(const std::filesystem::path &artifactPath)
        {
            if (!File::dir(artifactPath))
            {
                return "missing";
            }
            if (!File::exists(artifactPath / "artifact.toml"))
            {
                return "invalid";
            }

            DependencyArtifacts artifacts;
            NullDiagnosticSink diagnostics;
            if (!ArtifactStore::read(artifactPath / "artifact.toml", artifacts, diagnostics))
            {
                return "invalid";
            }

            BuildUtil::Context context;
            context.artifactDir = artifactPath;
            return BuildUtil::validate(context, artifacts, diagnostics) ? "built" : "invalid";
        }

        static bool visible(const LockedPackage &package, const DependencyOverrideSet &overrides)
        {
            const auto overrideIt = overrides.overrides.find(package.name);
            if (overrideIt == overrides.overrides.end())
            {
                return !package.overridden;
            }
            return package.overridden && package.overridePath == overrideIt->second.path.string();
        }
    };

    class PackageRow final
    {
    public:
        static std::filesystem::path sourcePath(const LockedPackage &package,
                                                const std::filesystem::path &workspaceRoot)
        {
            auto cachePath = std::filesystem::path{};
            if (package.sourceType == "local")
            {
                cachePath = std::filesystem::path(package.source);
            }
            else
            {
                DependencyDesc dependency;
                dependency.name = package.name;
                dependency.source = package.source;
                dependency.ref = package.ref;
                PackageCache cache(Path::stateRoot(workspaceRoot) / "packages");
                cachePath = cache.source(dependency);
            }
            if (package.sourceType == "local" && cachePath.is_relative())
            {
                cachePath = workspaceRoot / cachePath;
            }
            if (package.sourceType == "archive")
            {
                const auto key = package.sha256.empty() ? package.resolvedArchiveHash : package.sha256;
                cachePath = Path::stateRoot(workspaceRoot) / "packages" / "_archives" /
                            Path::sanitizePackagePath(package.name) / Path::sanitizePackagePath(key) / "src";
            }

            return cachePath;
        }

        static void details(const LockedPackage &package, const std::filesystem::path &cachePath,
                            const std::string &status)
        {
            LogInfo("  artifact_id={}", package.artifactId.empty() ? "-" : package.artifactId);
            LogInfo("  build_hash={}", package.buildHash.empty() ? "-" : package.buildHash);
            LogInfo("  source_type={}", package.sourceType.empty() ? "-" : package.sourceType);
            LogInfo("  overridden={}", package.overridden ? "yes" : "no");
            if (package.overridden)
            {
                LogInfo("  override_path={}", package.overridePath.empty() ? "-" : package.overridePath);
                LogInfo("  override_source={}", package.overrideSource.empty() ? "-" : package.overrideSource);
                LogInfo("  original_source={}", package.originalSource.empty() ? "-" : package.originalSource);
                LogInfo("  original_source_type={}",
                        package.originalSourceType.empty() ? "-" : package.originalSourceType);
                LogInfo("  original_ref={}", package.originalRef.empty() ? "-" : package.originalRef);
            }
            if (package.sourceType == "archive")
            {
                LogInfo("  sha256={}", package.sha256.empty() ? "-" : package.sha256);
                LogInfo("  resolved_archive_hash={}",
                        package.resolvedArchiveHash.empty() ? "-" : package.resolvedArchiveHash);
                LogInfo("  strip_components={}", package.stripComponents);
                LogInfo("  patches={}", package.patches.size());
                for (std::size_t i = 0; i < package.patchHashes.size(); ++i)
                {
                    LogInfo("  patch_hash[{}]={}", i, package.patchHashes[i]);
                }
                LogInfo("  source_dir={}", cachePath.string());
            }
            LogInfo("  toolchain_sensitive={}", package.buildHash.empty() ? "unknown" : "yes");
            LogInfo("  up_to_date_reason={}", status == "built"
                                                  ? "artifact metadata and exported files are valid"
                                                  : "artifact metadata or exported files are missing/invalid");
        }

        static void print(const LockedPackage &package, const std::filesystem::path &workspaceRoot, bool verbose)
        {
            const auto cachePath = sourcePath(package, workspaceRoot);
            const auto artifactPath = Path::stateRoot(workspaceRoot) / "artifacts" / package.artifactId;
            const auto status = PackageStatus::read(artifactPath);
            LogInfo("{} source={} ref={} commit={} build={} cache={} artifact={} status={}", package.name,
                    package.source, package.ref.empty() ? "-" : package.ref,
                    package.commit.empty() ? "-" : package.commit, package.buildType, cachePath.string(),
                    artifactPath.string(), status);
            if (verbose)
            {
                details(package, cachePath, status);
            }
        }
    };

    int Commands::packages(const std::filesystem::path &workspaceRoot, bool verbose,
                           const std::vector<std::string> &cliOverrides)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            return 1;
        }
        DependencyOverrideSet overrides;
        if (!CliWorkspace::overrides(model, workspaceRoot, cliOverrides, overrides, diagnostics))
        {
            return 1;
        }
        LockFile lockFile;
        if (!LockFile::read(Path::lockFile(workspaceRoot), lockFile, diagnostics))
        {
            return 1;
        }
        if (lockFile.packages.empty())
        {
            LogInfo("packages: none");
            return 0;
        }

        bool printed = false;
        for (const auto &package : lockFile.packages)
        {
            if (!PackageStatus::visible(package, overrides))
            {
                continue;
            }
            printed = true;
            PackageRow::print(package, workspaceRoot, verbose);
        }
        if (!printed)
        {
            LogInfo("packages: none");
        }
        return 0;
    }

    int Commands::packagesGc(const std::filesystem::path &workspaceRoot, const PackageGcOptions &request)
    {
        const auto &[apply, packageFilter, builds, sources, preferredToolchain, configOption] = request;
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        LockFile lockFile;
        if (!LockFile::read(Path::lockFile(workspaceRoot), lockFile, diagnostics))
        {
            return 1;
        }
        if (!ArtifactGc::runArtifacts(lockFile, workspaceRoot, apply, packageFilter, diagnostics))
        {
            return 1;
        }
        if (sources && !ArtifactGc::runSourceCaches(lockFile, workspaceRoot, apply, packageFilter, diagnostics))
        {
            return 1;
        }
        if (builds)
        {
            BuildModel model;
            if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
            {
                return 1;
            }
            const auto config = CliWorkspace::config(model, configOption, diagnostics);
            if (config.empty())
            {
                return 1;
            }
            const auto effectiveToolchain = CliWorkspace::toolchain(workspaceRoot, preferredToolchain, diagnostics);
            if (effectiveToolchain.empty())
            {
                return 1;
            }
            if (!ArtifactGc::runBuildCaches(lockFile, workspaceRoot,
                                            BuildCacheGcOptions{.config = config,
                                                                .toolchainId = effectiveToolchain,
                                                                .apply = apply,
                                                                .packageFilter = packageFilter},
                                            diagnostics))
            {
                return 1;
            }
        }
        return 0;
    }

} // namespace rhodium
