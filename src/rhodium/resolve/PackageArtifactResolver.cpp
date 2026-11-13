/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/Path.hpp"

#include "rhodium/resolve/PackageArtifactResolver.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PackageArtifactStore.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace rhodium
{
    bool PackageArtifactResolver::shouldLoad(const LockedPackage &locked, const std::string &config,
                                             const std::string &toolchainId, const DependencyOverrideSet &overrides)
    {
        if (BuildConfig::normalize(locked.config) != BuildConfig::normalize(config) ||
            locked.toolchainId != toolchainId)
        {
            return false;
        }
        const auto overrideIt = overrides.overrides.find(locked.name);
        if (overrideIt == overrides.overrides.end())
        {
            return !locked.overridden;
        }
        return locked.overridden && locked.overridePath == overrideIt->second.path.string();
    }

    void PackageArtifactResolver::appendDirs(std::vector<std::filesystem::path> &output,
                                             const std::filesystem::path &root, const std::vector<std::string> &values)
    {
        for (const auto &value : values)
        {
            auto path = std::filesystem::path(value);
            output.push_back(path.is_absolute() ? path : root / path);
        }
    }

    bool PackageArtifactResolver::load(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                       DiagnosticSink &diagnostics)
    {
        return load(lockFile, workspaceRoot, BuildSelection{"debug", {}}, DependencyOverrideSet{}, diagnostics);
    }

    bool PackageArtifactResolver::load(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                       const DependencyOverrideSet &overrides, DiagnosticSink &diagnostics)
    {
        return load(lockFile, workspaceRoot, BuildSelection{"debug", {}}, overrides, diagnostics);
    }

    bool PackageArtifactResolver::load(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                       const BuildSelection &selection, const DependencyOverrideSet &overrides,
                                       DiagnosticSink &diagnostics)
    {
        const auto &[config, toolchainId] = selection;
        packages_.clear();

        std::vector<const LockedPackage *> selectedPackages;
        std::unordered_map<std::string, std::size_t> selectedIndexByName;
        for (const auto &locked : lockFile.packages)
        {
            if (!shouldLoad(locked, config, toolchainId, overrides))
            {
                continue;
            }
            if (locked.name.empty() || locked.artifactId.empty())
            {
                diagnostics.error("lockfile package is missing name or artifact_id");
                return false;
            }

            const auto existing = selectedIndexByName.find(locked.name);
            if (existing != selectedIndexByName.end())
            {
                selectedPackages[existing->second] = &locked;
                continue;
            }
            selectedIndexByName[locked.name] = selectedPackages.size();
            selectedPackages.push_back(&locked);
        }

        for (const auto *lockedPackage : selectedPackages)
        {
            const auto &locked = *lockedPackage;
            const auto artifactRoot = Path::stateRoot(workspaceRoot) / "artifacts" / locked.artifactId;
            DependencyArtifacts artifacts;
            if (!ArtifactStore::read(artifactRoot / "artifact.toml", artifacts, diagnostics))
            {
                return false;
            }
            BuildUtil::Context context;
            context.artifactDir = artifactRoot;
            if (!BuildUtil::validate(context, artifacts, diagnostics))
            {
                diagnostics.error("package artifact is invalid; run rhodium restore: " + locked.name);
                return false;
            }

            ResolvedDependencyArtifact resolved;
            resolved.name = locked.name;
            resolved.root = artifactRoot;
            appendDirs(resolved.includeDirs, artifactRoot, artifacts.includeDirs);
            appendDirs(resolved.libDirs, artifactRoot, artifacts.libDirs);
            appendDirs(resolved.binDirs, artifactRoot, artifacts.binDirs);
            appendDirs(resolved.binFiles, artifactRoot, artifacts.binFiles);
            resolved.libs = artifacts.libs;
            appendDirs(resolved.libFiles, artifactRoot, artifacts.libFiles);
            resolved.defines = artifacts.defines;
            resolved.systemLibs = artifacts.systemLibs;
            resolved.frameworks = artifacts.frameworks;
            packages_[resolved.name] = std::move(resolved);
        }
        return true;
    }

    const ResolvedDependencyArtifact *PackageArtifactResolver::find(const std::string &name) const
    {
        const auto it = packages_.find(name);
        return it == packages_.end() ? nullptr : &it->second;
    }

    std::vector<ResolvedDependencyArtifact> PackageArtifactResolver::packages() const
    {
        std::vector<ResolvedDependencyArtifact> result;
        for (const auto &[_, package] : packages_)
        {
            result.push_back(package);
        }
        std::sort(result.begin(), result.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                      return lhs.name < rhs.name;
                  });
        return result;
    }

} // namespace rhodium
