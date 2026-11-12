#include "rhodium/core/WorkspaceLock.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/package/ToolchainContextResolver.hpp"
#include <set>
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

    namespace
    {
        class RestoreSession final
        {
        public:
            RestoreSession(const BuildModel &model, const std::filesystem::path &workspaceRoot,
                           DiagnosticSink &diagnostics, const DependencyRestoreRequest &request)
                : model(model), workspaceRoot(workspaceRoot), diagnostics(diagnostics),
                  preferredToolchain(request.preferredToolchain),
                  effectiveConfig(BuildConfig::normalize(request.config, model.workspace.defaultConfig)),
                  overrides(request.overrides), options(request.options),
                  cache(Path::stateRoot(workspaceRoot) / "packages"), lockPath(workspaceRoot / "tpkg.lock.toml")
            {
            }

            bool run()
            {

                if (options.exportOnly && options.rebuild)
                {
                    diagnostics.error("restore --export-only cannot be combined with --rebuild");
                    return false;
                }
                if (options.locked && (options.updateAll || !options.updatePackage.empty()))
                {
                    diagnostics.error("restore --locked cannot update dependency versions");
                    return false;
                }

                if (!BuildConfig::validate(effectiveConfig, diagnostics))
                {
                    return false;
                }

                std::string error;
                if (!File::mkdir(workspaceRoot, &error))
                {
                    diagnostics.error("failed to create workspace root: " + error);
                    return false;
                }
                auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
                if (!workspaceLock)
                {
                    return false;
                }
                if (!cache.mkdir(&error))
                {
                    diagnostics.error("failed to create source cache: " + error);
                    return false;
                }

                if (!LockFile::read(lockPath, lockFile, diagnostics))
                {
                    return false;
                }

                std::vector<DependencyDesc> allDependencies;
                if (!DepGraph::build(model, allDependencies, diagnostics))
                {
                    return false;
                }

                ToolchainSnapshot localSnapshot(workspaceRoot);
                restoreOptions = options;
                if (!restoreOptions.toolchains)
                    restoreOptions.toolchains = &localSnapshot;
                for (const auto &dependency : allDependencies)
                {
                    if (!restorePackage(dependency))
                    {
                        return false;
                    }
                }
                return finishRestore(allDependencies);
            }

        private:
            struct PackageState
            {
                LockedPackage *locked;
                bool wantsUpdate;
                std::string oldCommit;
                std::string oldHash;
            };

            bool finishRestore(const std::vector<DependencyDesc> &allDependencies)
            {
                if (!options.updatePackage.empty())
                {
                    const auto found =
                        std::find_if(model.rootPackage.dependencies.begin(), model.rootPackage.dependencies.end(),
                                     [&](const DependencyDesc &dependency)
                                     {
                                         return dependency.name == options.updatePackage;
                                     });
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
                if (options.locked)
                {
                    return true;
                }
                const auto selectedPackage = !options.packageName.empty() ? options.packageName : options.updatePackage;
                if (selectedPackage.empty())
                {
                    std::set<std::string> activeNames;
                    for (const auto &dependency : allDependencies)
                    {
                        activeNames.insert(dependency.name);
                    }
                    lockFile.packages.erase(
                        std::remove_if(lockFile.packages.begin(), lockFile.packages.end(),
                                       [&](const LockedPackage &entry)
                                       {
                                           return activeNames.find(entry.name) == activeNames.end() &&
                                                  BuildConfig::normalize(entry.config) == effectiveConfig &&
                                                  (entry.toolchainId == preferredToolchain ||
                                                   (!preferredToolchain.empty() && entry.toolchainId.empty() &&
                                                    effectiveConfig == "debug"));
                                       }),
                        lockFile.packages.end());
                }
                if (!LockFile::write(lockPath, lockFile, diagnostics))
                {
                    return false;
                }
                return options.buildOnly ||
                       DepExport::gc(lockFile, workspaceRoot, BuildSelection{effectiveConfig, preferredToolchain},
                                     selectedPackage, diagnostics);
            }

            void addPrefixes(DependencyDesc &dependency)
            {
                if (!dependency.dependencies.empty())
                {
                    std::vector<std::string> prefixPaths;
                    for (const auto &depName : dependency.dependencies)
                    {
                        const auto activeOverride = overrides.overrides.find(depName);
                        for (const auto &entry : lockFile.packages)
                        {
                            if (entry.overridden != (activeOverride != overrides.overrides.end()) ||
                                (entry.overridden && entry.overridePath != activeOverride->second.path.string()))
                            {
                                continue;
                            }
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
                            if (i > 0)
                                prefixArg += ";";
                            prefixArg += prefixPaths[i];
                        }
                        dependency.cmake.configureArgs.insert(dependency.cmake.configureArgs.begin(), prefixArg);
                    }
                }
            }

            void applyOverride(DependencyDesc &dependency)
            {
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
            }

            bool restorePackage(const DependencyDesc &originalDependency)
            {
                auto dependency = originalDependency;
                DepBuild::applyConfig(dependency, effectiveConfig);
                addPrefixes(dependency);
                applyOverride(dependency);
                diagnostics.info("restoring package " + dependency.name + " from " + dependency.source +
                                 " build=" + dependency.buildType);
                auto lockDependency = dependency;
                lockDependency.source = DepLock::source(workspaceRoot, dependency);
                auto *locked = DepLock::find(lockFile, lockDependency, effectiveConfig, preferredToolchain);
                const bool wantsUpdate =
                    !dependency.overridden &&
                    (options.updateAll || (!options.updatePackage.empty() && options.updatePackage == dependency.name));
                const auto selectedPackage = !options.packageName.empty() ? options.packageName : options.updatePackage;
                if (!selectedPackage.empty() && selectedPackage != dependency.name)
                {
                    return true;
                }
                processedSelectedPackage = true;
                if (options.updateAll && dependency.overridden)
                {
                    diagnostics.info("skipping overridden package during update --all: " + dependency.name);
                    return true;
                }
                if (options.locked && !locked)
                {
                    diagnostics.error("restore --locked requires an existing lock entry for package " +
                                      dependency.name + " config=" + effectiveConfig +
                                      " toolchain=" + preferredToolchain);
                    return false;
                }
                const auto oldCommit = locked ? locked->commit : std::string{};
                const auto oldHash = locked ? locked->buildHash : std::string{};

                PackageState state{locked, wantsUpdate, oldCommit, oldHash};
                DepFetch::Result fetchResult;
                DepBuild::Result buildResult;
                if (!buildPackage(dependency, locked, wantsUpdate, fetchResult, buildResult))
                {
                    return false;
                }
                if (fetchResult.skipPackage || buildResult.skipPackage)
                {
                    return true;
                }
                return finish(dependency, state, fetchResult, buildResult);
            }

            bool buildPackage(DependencyDesc &dependency, LockedPackage *locked, bool wantsUpdate,
                              DepFetch::Result &fetchResult, DepBuild::Result &buildResult)
            {
                DepFetch::Context fetchContext{workspaceRoot,      cache,         git, lockFile, effectiveConfig,
                                               preferredToolchain, restoreOptions};
                if (!DepFetch::source(
                        DepFetch::Request{.dependency = dependency, .locked = locked, .wantsUpdate = wantsUpdate},
                        fetchContext, fetchResult, diagnostics))
                {
                    return false;
                }
                if (fetchResult.skipPackage)
                {
                    if (!options.locked)
                        DepLock::store(lockFile, *locked, locked);
                    return true;
                }

                DepBuild::Context buildContext{workspaceRoot,      cache, effectiveConfig,
                                               preferredToolchain, restoreOptions};
                if (!DepBuild::run(DepBuild::Request{.dependency = dependency,
                                                     .sourceRoot = fetchResult.sourceRoot,
                                                     .commit = fetchResult.commit,
                                                     .locked = locked,
                                                     .wantsUpdate = wantsUpdate},
                                   buildContext, buildResult, diagnostics))
                {
                    return false;
                }
                if (buildResult.skipPackage)
                {
                    if (!options.locked)
                        DepLock::store(lockFile, *locked, locked);
                    return true;
                }

                return true;
            }

            bool finish(const DependencyDesc &dependency, PackageState &state, const DepFetch::Result &fetchResult,
                        DepBuild::Result &buildResult)
            {
                auto &[locked, wantsUpdate, oldCommit, oldHash] = state;
                if (options.buildOnly)
                {
                    if (!options.locked)
                    {
                        if (locked && locked->buildHash == buildResult.updated.buildHash &&
                            locked->artifactId == buildResult.updated.artifactId)
                        {
                            buildResult.updated.artifacts = locked->artifacts;
                        }
                        DepLock::store(lockFile, std::move(buildResult.updated), locked);
                        if (!LockFile::write(lockPath, lockFile, diagnostics))
                        {
                            return false;
                        }
                    }
                    diagnostics.info("package build stage completed without artifact export: " + dependency.name);
                    return true;
                }
                if (!DepExport::write(buildResult.artifact, buildResult.updated, buildResult.artifactDir, diagnostics))
                {
                    return false;
                }

                if (options.locked)
                {
                    diagnostics.info("restore --locked used " + dependency.name + " commit=" +
                                     buildResult.updated.commit + " build_hash=" + buildResult.updated.buildHash);
                    return true;
                }
                const auto newHash = buildResult.updated.buildHash;
                DepLock::store(lockFile, std::move(buildResult.updated), locked);

                if (wantsUpdate)
                {
                    diagnostics.info(
                        "updated package " + dependency.name + " old_commit=" + (oldCommit.empty() ? "-" : oldCommit) +
                        " new_commit=" + fetchResult.commit + " old_hash=" + (oldHash.empty() ? "-" : oldHash) +
                        " new_hash=" + newHash + " rebuild_artifact=yes");
                }

                if (!LockFile::write(lockPath, lockFile, diagnostics))
                {
                    return false;
                }

                return true;
            }

            const BuildModel &model;
            const std::filesystem::path &workspaceRoot;
            DiagnosticSink &diagnostics;
            const std::string &preferredToolchain;
            const std::string effectiveConfig;
            const DependencyOverrideSet &overrides;
            const DependencyRestoreOptions &options;
            DependencyRestoreOptions restoreOptions;
            PackageCache cache;
            LockFile lockFile;
            const std::filesystem::path lockPath;
            GitFetcher git;
            bool processedSelectedPackage = false;
        };
    }

    bool DependencyResolver::restore(const BuildModel &model, const std::filesystem::path &workspaceRoot,
                                     DiagnosticSink &diagnostics, const DependencyRestoreRequest &request) const
    {
        return RestoreSession(model, workspaceRoot, diagnostics, request).run();
    }

} // namespace rhodium
