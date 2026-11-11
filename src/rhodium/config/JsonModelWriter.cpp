/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/config/JsonModelWriter.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

namespace rhodium
{
    std::vector<std::string> JsonModel::paths(const std::vector<std::filesystem::path> &paths)
    {
        std::vector<std::string> result;
        for (const auto &path : paths)
        {
            result.push_back(path.generic_string());
        }
        return result;
    }

    nlohmann::json JsonModel::dependency(const DependencyDesc &desc)
    {
        return {
            {"name", desc.name},
            {"source", desc.source},
            {"source_type", desc.sourceType},
            {"sha256", desc.sha256},
            {"resolved_archive_hash", desc.resolvedArchiveHash},
            {"ref", desc.ref},
            {"subdir", desc.subdir},
            {"strip_components", desc.stripComponents},
            {"patches", desc.patches},
            {"patch_hashes", desc.patchHashes},
            {"build", desc.buildType},
            {"linkage", desc.linkage},
            {"runtime", desc.runtime},
            {"pic", desc.pic},
            {"toolchain_requirements", {
                {"family", desc.toolchainRequirements.family},
                {"abi", desc.toolchainRequirements.abi},
                {"binary_format", desc.toolchainRequirements.binaryFormat},
            }},
            {"build_options", desc.buildOptions},
            {"cmake", {
                {"generator", desc.cmake.generator},
                {"build_type", desc.cmake.buildType},
                {"options", desc.cmake.options},
                {"configure_args", desc.cmake.configureArgs},
                {"build_args", desc.cmake.buildArgs},
                {"build_targets", desc.cmake.buildTargets},
                {"install_args", desc.cmake.installArgs},
                {"env", desc.cmake.env},
                {"toolchain_file", desc.cmake.toolchainFile},
                {"install", desc.cmake.install},
                {"install_target", desc.cmake.installTarget},
            }},
            {"make", {
                {"executable", desc.make.executable},
                {"jobs", desc.make.jobs},
                {"args", desc.make.args},
                {"targets", desc.make.targets},
                {"install_targets", desc.make.installTargets},
                {"install_args", desc.make.installArgs},
                {"env", desc.make.env},
            }},
            {"configure", {
                {"script", desc.configure.script},
                {"args", desc.configure.args},
                {"env", desc.configure.env},
            }},
            {"commands", {
                {"configure", desc.commands.configure},
                {"build", desc.commands.build},
                {"install", desc.commands.install},
            }},
            {"artifacts", {
                {"mode", desc.artifacts.mode},
                {"include_dirs", desc.artifacts.includeDirs},
                {"lib_dirs", desc.artifacts.libDirs},
                {"bin_dirs", desc.artifacts.binDirs},
                {"bin_files", desc.artifacts.binFiles},
                {"libs", desc.artifacts.libs},
                {"lib_files", desc.artifacts.libFiles},
                {"lib_files_by_config", desc.artifacts.libFilesByConfig},
                {"defines", desc.artifacts.defines},
                {"system_libs", desc.artifacts.systemLibs},
                {"frameworks", desc.artifacts.frameworks},
            }},
            {"local", desc.local},
        };
    }

    nlohmann::json JsonModel::from(const BuildModel &model)
    {
        nlohmann::json dependencies = nlohmann::json::array();
        for (const auto &dependency : model.rootPackage.dependencies)
        {
            dependencies.push_back(JsonModel::dependency(dependency));
        }

        nlohmann::json dependencyOverrides = nlohmann::json::array();
        for (const auto &override : model.rootPackage.dependencyOverrides)
        {
            dependencyOverrides.push_back({
                {"name", override.name},
                {"path", override.path},
            });
        }

        return {
            {"manifest", model.manifestName},
            {"package", {
                {"name", model.rootPackage.name},
                {"version", model.rootPackage.version},
                {"dependencies", dependencies},
                {"dependency_overrides", dependencyOverrides},
            }},
            {"defaults", {
                {"config", model.workspace.defaultConfig},
                {"platform", model.workspace.defaultPlatform},
                {"arch", model.workspace.defaultArch},
                {"generate_cmake_user_presets", model.workspace.generateCMakeUserPresets},
            }},
        };
    }

    std::string JsonModel::write(const BuildModel &model)
    {
        return from(model).dump(2);
    }

    nlohmann::json JsonModel::from(const ResolvedDependencyArtifacts &model)
    {
        nlohmann::json packages = nlohmann::json::array();
        for (const auto &package : model.packages)
        {
            packages.push_back({
                {"name", package.name},
                {"root", package.root.generic_string()},
                {"include_dirs", paths(package.includeDirs)},
                {"lib_dirs", paths(package.libDirs)},
                {"bin_dirs", paths(package.binDirs)},
                {"bin_files", paths(package.binFiles)},
                {"libs", package.libs},
                {"lib_files", paths(package.libFiles)},
                {"defines", package.defines},
                {"system_libs", package.systemLibs},
                {"frameworks", package.frameworks},
            });
        }
        return {{"packages", packages}};
    }

    std::string JsonModel::write(const ResolvedDependencyArtifacts &model)
    {
        return from(model).dump(2);
    }

} // namespace rhodium
