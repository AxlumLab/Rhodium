#include "rhodium/core/Path.hpp"

#include "rhodium/resolve/DependencyResolver.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/config/LockFile.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/GitFetcher.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/resolve/DependencyBuildStep.hpp"
#include "rhodium/resolve/DependencyExportStep.hpp"
#include "rhodium/resolve/DependencyFetchStep.hpp"
#include "rhodium/resolve/DependencyGraph.hpp"
#include "rhodium/resolve/DependencyLockStep.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace rhodium
{

    bool DependencyResolver::restore(const BuildModel &model,
                                     const std::filesystem::path &workspaceRoot,
                                     DiagnosticSink &diagnostics,
                                     const std::string &preferredToolchain,
                                     const std::string &config,
                                     const DependencyOverrideSet &overrides,
                                     const DependencyRestoreOptions &options) const
    {
        const auto effectiveConfig = BuildConfig::normalize(config, model.workspace.defaultConfig);
        if (!BuildConfig::validate(effectiveConfig, diagnostics))
        {
            return false;
        }

        PackageCache cache(Path::stateRoot(workspaceRoot) / "packages");
        std::string error;
        if (!cache.mkdir(&error))
        {
            diagnostics.error("failed to create source cache: " + error);
            return false;
        }

        LockFile lockFile;
        const auto lockPath = workspaceRoot / "tpkg.lock.toml";
        if (!LockFile::read(lockPath, lockFile, diagnostics))
        {
            return false;
        }

        std::vector<DependencyDesc> allDependencies;
        if (!DepGraph::build(model, allDependencies, diagnostics))
        {
            return false;
        }

        GitFetcher git;
        bool processedSelectedPackage = false;
        for (const auto &originalDependency : allDependencies)
        {
            auto dependency = originalDependency;
            DepBuild::applyConfig(dependency, effectiveConfig);

            if (!dependency.dependencies.empty())
            {
                std::vector<std::string> prefixPaths;
                for (const auto &depName : dependency.dependencies)
                {
                    for (const auto &entry : lockFile.packages)
                    {
                        if (entry.name == depName &&
                            BuildConfig::normalize(entry.config) == BuildConfig::normalize(effectiveConfig) &&
                            entry.toolchainId == preferredToolchain)
                        {
                            auto depArtifactDir = Path::stateRoot(workspaceRoot) / "artifacts" / entry.artifactId;
                            if (std::filesystem::is_directory(depArtifactDir))
                            {
                                prefixPaths.push_back(depArtifactDir.string());
                            }
                            break;
                        }
                    }
                }
                if (!prefixPaths.empty())
                {
                    std::string prefixArg = "-DCMAKE_PREFIX_PATH=";
                    for (size_t i = 0; i < prefixPaths.size(); ++i)
                    {
                        if (i > 0) prefixArg += ";";
                        prefixArg += prefixPaths[i];
                    }
                    dependency.cmake.configureArgs.insert(dependency.cmake.configureArgs.begin(), prefixArg);
                }
            }

            const auto overrideIt = overrides.overrides.find(dependency.name);
            if (overrideIt != overrides.overrides.end())
            {
                dependency.overridden = true;
                dependency.overridePath = overrideIt->second.path.string();
                dependency.overrideSource = overrideIt->second.source;
                dependency.originalSource = dependency.source;
                dependency.originalSourceType = dependency.sourceType;
                dependency.originalRef = dependency.ref;
                dependency.source = dependency.overridePath;
                dependency.sourceType = "local";
                dependency.local = true;
                dependency.sha256.clear();
                dependency.resolvedArchiveHash.clear();
                dependency.ref.clear();
            }
            diagnostics.info("restoring package " + dependency.name + " from " + dependency.source + " build=" + dependency.buildType);
            auto *locked = DepLock::find(lockFile, dependency, effectiveConfig, preferredToolchain);
            const bool wantsUpdate = !dependency.overridden &&
                                     (options.updateAll || (!options.updatePackage.empty() && options.updatePackage == dependency.name));
            const auto selectedPackage = !options.packageName.empty() ? options.packageName : options.updatePackage;
            if (!selectedPackage.empty() && selectedPackage != dependency.name)
            {
                continue;
            }
            processedSelectedPackage = true;
            if (options.updateAll && dependency.overridden)
            {
                diagnostics.info("skipping overridden package during update --all: " + dependency.name);
                continue;
            }
            if (options.locked && !locked)
            {
                diagnostics.error("restore --locked requires an existing lock entry for package " + dependency.name +
                                  " config=" + effectiveConfig + " toolchain=" + preferredToolchain);
                return false;
            }
            const auto oldCommit = locked ? locked->commit : std::string{};
            const auto oldHash = locked ? locked->buildHash : std::string{};

            DepFetch::Result fetchResult;
            DepFetch::Context fetchContext{
                workspaceRoot,
                cache,
                git,
                lockFile,
                effectiveConfig,
                preferredToolchain,
                options};
            if (!DepFetch::source(dependency, locked, wantsUpdate, fetchContext, fetchResult, diagnostics))
            {
                return false;
            }
            if (fetchResult.skipPackage)
            {
                continue;
            }

            DepBuild::Result buildResult;
            DepBuild::Context buildContext{
                workspaceRoot,
                cache,
                lockFile,
                effectiveConfig,
                preferredToolchain,
                options};
            if (!DepBuild::run(dependency,
                               fetchResult.sourceRoot,
                               fetchResult.commit,
                               locked,
                               wantsUpdate,
                               buildContext,
                               buildResult,
                               diagnostics))
            {
                return false;
            }
            if (buildResult.skipPackage)
            {
                continue;
            }

            if (options.buildOnly)
            {
                if (!options.locked)
                {
                    if (locked)
                    {
                        *locked = buildResult.updated;
                    }
                    else
                    {
                        lockFile.packages.push_back(buildResult.updated);
                    }
                    if (!LockFile::write(lockPath, lockFile, diagnostics))
                    {
                        return false;
                    }
                }
                diagnostics.info("package build stage completed without artifact export: " + dependency.name);
                continue;
            }
            if (!DepExport::write(buildResult.artifact,
                                  buildResult.updated,
                                  buildResult.artifactDir,
                                  diagnostics))
            {
                return false;
            }

            if (options.locked)
            {
                diagnostics.info("restore --locked used " + dependency.name + " commit=" + buildResult.updated.commit + " build_hash=" + buildResult.updated.buildHash);
                continue;
            }
            if (locked)
            {
                *locked = std::move(buildResult.updated);
            }
            else
            {
                lockFile.packages.push_back(std::move(buildResult.updated));
            }

            if (wantsUpdate)
            {
                diagnostics.info("updated package " + dependency.name +
                                 " old_commit=" + (oldCommit.empty() ? "-" : oldCommit) +
                                 " new_commit=" + fetchResult.commit +
                                 " old_hash=" + (oldHash.empty() ? "-" : oldHash) +
                                 " new_hash=" + (locked ? locked->buildHash : lockFile.packages.back().buildHash) +
                                 " rebuild_artifact=yes");
            }

            if (!LockFile::write(lockPath, lockFile, diagnostics))
            {
                return false;
            }
            if (!options.buildOnly)
            {
                if (!DepExport::gc(lockFile, workspaceRoot, effectiveConfig, preferredToolchain, dependency.name, diagnostics))
                {
                    return false;
                }
            }
        }

        if (!options.updatePackage.empty())
        {
            const auto found = std::find_if(model.rootPackage.dependencies.begin(), model.rootPackage.dependencies.end(), [&](const DependencyDesc &dependency)
                                           { return dependency.name == options.updatePackage; });
            if (found == model.rootPackage.dependencies.end())
            {
                diagnostics.error("update package is not declared: " + options.updatePackage);
                return false;
            }
        }
        if (!options.packageName.empty() && !processedSelectedPackage)
        {
            diagnostics.error("restore package is not declared: " + options.packageName);
            return false;
        }
        return options.locked ? true : LockFile::write(lockPath, lockFile, diagnostics);
    }

} // namespace rhodium
