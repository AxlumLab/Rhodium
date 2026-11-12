/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/config/LockFile.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Hash.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <toml++/toml.hpp>
#include <vector>

namespace rhodium
{
    class LockData final
    {
    public:
        static std::vector<std::string> strings(const toml::table &table, const char *key)
        {
            std::vector<std::string> values;
            const auto *array = table[key].as_array();
            if (!array)
            {
                return values;
            }
            for (const auto &item : *array)
            {
                if (auto value = item.value<std::string>())
                {
                    values.push_back(*value);
                }
            }
            return values;
        }

        static void writeStrings(std::ostringstream &stream, const char *key, const std::vector<std::string> &values)
        {
            stream << key << " = [";
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                if (i != 0)
                {
                    stream << ", ";
                }
                stream << toml::value<std::string>(values[i]);
            }
            stream << "]\n";
        }

        static std::string hash(const DependencyDesc &dependency, const std::string &commit, const std::string &config,
                                const std::string &toolchainId, const std::string &toolchainContentHash)
        {
            std::ostringstream stream;
            stream << "name=" << dependency.name << "\n";
            stream << "source=" << dependency.source << "\n";
            stream << "source_type=" << dependency.sourceType << "\n";
            stream << "overridden=" << (dependency.overridden ? "true" : "false") << "\n";
            stream << "override_path=" << dependency.overridePath << "\n";
            stream << "override_source=" << dependency.overrideSource << "\n";
            stream << "original_source=" << dependency.originalSource << "\n";
            stream << "original_source_type=" << dependency.originalSourceType << "\n";
            stream << "original_ref=" << dependency.originalRef << "\n";
            stream << "sha256=" << dependency.sha256 << "\n";
            stream << "resolved_archive_hash=" << dependency.resolvedArchiveHash << "\n";
            stream << "ref=" << dependency.ref << "\n";
            stream << "commit=" << commit << "\n";
            stream << "subdir=" << dependency.subdir << "\n";
            stream << "strip_components=" << dependency.stripComponents << "\n";
            appendStrings(stream, "patches", dependency.patches);
            appendStrings(stream, "patch_hashes", dependency.patchHashes);
            stream << "config=" << config << "\n";
            stream << "toolchain=" << toolchainId << "\n";
            stream << "toolchain_content=" << toolchainContentHash << "\n";
            stream << "build=" << dependency.buildType << "\n";
            stream << "linkage=" << dependency.linkage << "\n";
            stream << "runtime=" << dependency.runtime << "\n";
            stream << "pic=" << dependency.pic << "\n";
            appendMap(stream, "build_options", dependency.buildOptions);
            stream << "cmake.generator=" << dependency.cmake.generator << "\n";
            stream << "cmake.build_type=" << dependency.cmake.buildType << "\n";
            stream << "cmake.toolchain_file=" << dependency.cmake.toolchainFile << "\n";
            stream << "cmake.install=" << (dependency.cmake.install ? "true" : "false") << "\n";
            stream << "cmake.install_target=" << dependency.cmake.installTarget << "\n";
            appendMap(stream, "cmake.options", dependency.cmake.options);
            appendMap(stream, "cmake.env", dependency.cmake.env);
            appendStrings(stream, "cmake.configure_args", dependency.cmake.configureArgs);
            appendStrings(stream, "cmake.build_args", dependency.cmake.buildArgs);
            appendStrings(stream, "cmake.build_targets", dependency.cmake.buildTargets);
            appendStrings(stream, "cmake.install_args", dependency.cmake.installArgs);
            stream << "make.executable=" << dependency.make.executable << "\n";
            stream << "make.jobs=" << dependency.make.jobs << "\n";
            appendMap(stream, "make.env", dependency.make.env);
            appendStrings(stream, "make.args", dependency.make.args);
            appendStrings(stream, "make.targets", dependency.make.targets);
            appendStrings(stream, "make.install_targets", dependency.make.installTargets);
            appendStrings(stream, "make.install_args", dependency.make.installArgs);
            stream << "configure.script=" << dependency.configure.script << "\n";
            appendMap(stream, "configure.env", dependency.configure.env);
            appendStrings(stream, "configure.args", dependency.configure.args);
            appendStrings(stream, "commands.configure", dependency.commands.configure);
            appendStrings(stream, "commands.build", dependency.commands.build);
            appendStrings(stream, "commands.install", dependency.commands.install);
            stream << "toolchain.family=" << dependency.toolchainRequirements.family << "\n";
            stream << "toolchain.abi=" << dependency.toolchainRequirements.abi << "\n";
            stream << "toolchain.binary_format=" << dependency.toolchainRequirements.binaryFormat << "\n";
            appendArtifacts(stream, dependency.artifacts);
            return Hash::xxhash64Hex(stream.str());
        }

        static LockedPackage read(const toml::table &table)
        {
            LockedPackage package;
            package.name = (table)["name"].value_or("");
            package.source = (table)["source"].value_or("");
            package.sourceType = (table)["type"].value_or("");
            if (package.sourceType.empty())
            {
                package.sourceType = (table)["source_type"].value_or("git");
            }
            package.sha256 = (table)["sha256"].value_or("");
            package.resolvedArchiveHash = (table)["resolved_archive_hash"].value_or("");
            package.ref = (table)["ref"].value_or("");
            package.commit = (table)["commit"].value_or("");
            package.subdir = (table)["subdir"].value_or(".");
            package.stripComponents = (table)["strip_components"].value_or(0);
            package.patches = LockData::strings(table, "patches");
            package.patchHashes = LockData::strings(table, "patch_hashes");
            package.buildType = (table)["build"].value_or("header_only");
            package.linkage = (table)["linkage"].value_or("default");
            package.runtime = (table)["runtime"].value_or("default");
            package.pic = (table)["pic"].value_or("default");
            package.config = (table)["config"].value_or("debug");
            package.toolchainId = (table)["toolchain"].value_or("");
            package.buildHash = (table)["build_hash"].value_or("");
            package.artifactId = (table)["artifact_id"].value_or("");
            if (package.artifactId.empty())
            {
                package.artifactId = (table)["cache_path"].value_or("");
            }
            package.overridden = (table)["overridden"].value_or(false);
            package.overridePath = (table)["override_path"].value_or("");
            package.overrideSource = (table)["override_source"].value_or("");
            package.originalSource = (table)["original_source"].value_or("");
            package.originalSourceType = (table)["original_source_type"].value_or("");
            package.originalRef = (table)["original_ref"].value_or("");

            if (const auto *cmake = (table)["cmake"].as_table())
            {
                if (const auto *options = (*cmake)["options"].as_table())
                {
                    for (const auto &[key, value] : *options)
                    {
                        if (auto option = value.value<std::string>())
                        {
                            package.buildOptions[std::string(key.str())] = *option;
                        }
                    }
                }
            }

            if (const auto *artifacts = (table)["artifacts"].as_table())
            {
                package.artifacts.mode = (*artifacts)["mode"].value_or("install");
                package.artifacts.includeDirs = LockData::strings(*artifacts, "include_dirs");
                package.artifacts.libDirs = LockData::strings(*artifacts, "lib_dirs");
                package.artifacts.binDirs = LockData::strings(*artifacts, "bin_dirs");
                package.artifacts.binFiles = LockData::strings(*artifacts, "bin_files");
                package.artifacts.libs = LockData::strings(*artifacts, "libs");
                package.artifacts.libFiles = LockData::strings(*artifacts, "lib_files");
                package.artifacts.defines = LockData::strings(*artifacts, "defines");
                package.artifacts.systemLibs = LockData::strings(*artifacts, "system_libs");
                package.artifacts.frameworks = LockData::strings(*artifacts, "frameworks");
            }
            return package;
        }

    private:
        static void appendStrings(std::ostringstream &stream, const char *key, const std::vector<std::string> &values)
        {
            stream << key << "=";
            for (const auto &value : values)
            {
                stream << value.size() << ":" << value << ";";
            }
            stream << "\n";
        }

        static void appendMap(std::ostringstream &stream, const char *key,
                              const std::map<std::string, std::string> &values)
        {
            stream << key << "=";
            for (const auto &[mapKey, value] : values)
            {
                stream << mapKey.size() << ":" << mapKey << "=" << value.size() << ":" << value << ";";
            }
            stream << "\n";
        }

        static void appendArtifacts(std::ostringstream &stream, const DependencyArtifacts &artifacts)
        {
            stream << "artifacts.mode=" << artifacts.mode << "\n";
            appendStrings(stream, "artifacts.include_dirs", artifacts.includeDirs);
            appendStrings(stream, "artifacts.lib_dirs", artifacts.libDirs);
            appendStrings(stream, "artifacts.bin_dirs", artifacts.binDirs);
            appendStrings(stream, "artifacts.bin_files", artifacts.binFiles);
            appendStrings(stream, "artifacts.libs", artifacts.libs);
            appendStrings(stream, "artifacts.lib_files", artifacts.libFiles);
            for (const auto &[config, files] : artifacts.libFilesByConfig)
            {
                const auto key = "artifacts.lib_files." + config;
                appendStrings(stream, key.c_str(), files);
            }
            appendStrings(stream, "artifacts.defines", artifacts.defines);
            appendStrings(stream, "artifacts.system_libs", artifacts.systemLibs);
            appendStrings(stream, "artifacts.frameworks", artifacts.frameworks);
        }
    };

    bool LockFile::read(const std::filesystem::path &path, LockFile &lockFile, DiagnosticSink &diagnostics)
    {
        if (!File::exists(path))
        {
            return true;
        }

        try
        {
            auto table = toml::parse_file(path.string());
            lockFile.version = table["version"].value_or(1);
            lockFile.packages.clear();

            auto packages = table["package"].as_array();
            if (!packages)
            {
                return true;
            }

            for (const auto &node : *packages)
            {
                const auto *packageTable = node.as_table();
                if (!packageTable)
                {
                    continue;
                }

                lockFile.packages.push_back(LockData::read(*packageTable));
            }
            return true;
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to read lockfile: " + std::string(ex.what()));
            return false;
        }
    }

    bool LockFile::write(const std::filesystem::path &path, const LockFile &lockFile, DiagnosticSink &diagnostics)
    {
        std::ostringstream stream;
        stream << "version = " << lockFile.version << "\n\n";
        for (const auto &package : lockFile.packages)
        {
            stream << "[[package]]\n";
            stream << "name = " << toml::value<std::string>(package.name) << "\n";
            stream << "source = " << toml::value<std::string>(package.source) << "\n";
            stream << "type = " << toml::value<std::string>(package.sourceType) << "\n";
            stream << "overridden = " << (package.overridden ? "true" : "false") << "\n";
            stream << "override_path = " << toml::value<std::string>(package.overridePath) << "\n";
            stream << "override_source = " << toml::value<std::string>(package.overrideSource) << "\n";
            stream << "original_source = " << toml::value<std::string>(package.originalSource) << "\n";
            stream << "original_source_type = " << toml::value<std::string>(package.originalSourceType) << "\n";
            stream << "original_ref = " << toml::value<std::string>(package.originalRef) << "\n";
            stream << "sha256 = " << toml::value<std::string>(package.sha256) << "\n";
            stream << "resolved_archive_hash = " << toml::value<std::string>(package.resolvedArchiveHash) << "\n";
            stream << "ref = " << toml::value<std::string>(package.ref) << "\n";
            stream << "commit = " << toml::value<std::string>(package.commit) << "\n";
            stream << "subdir = " << toml::value<std::string>(package.subdir) << "\n";
            stream << "strip_components = " << package.stripComponents << "\n";
            LockData::writeStrings(stream, "patches", package.patches);
            LockData::writeStrings(stream, "patch_hashes", package.patchHashes);
            stream << "build = " << toml::value<std::string>(package.buildType) << "\n";
            stream << "linkage = " << toml::value<std::string>(package.linkage) << "\n";
            stream << "runtime = " << toml::value<std::string>(package.runtime) << "\n";
            stream << "pic = " << toml::value<std::string>(package.pic) << "\n";
            stream << "config = " << toml::value<std::string>(package.config.empty() ? "debug" : package.config)
                   << "\n";
            stream << "toolchain = " << toml::value<std::string>(package.toolchainId) << "\n";
            stream << "build_hash = " << toml::value<std::string>(package.buildHash) << "\n";
            stream << "artifact_id = " << toml::value<std::string>(package.artifactId) << "\n";
            if (!package.buildOptions.empty())
            {
                stream << "\n[package.cmake.options]\n";
                for (const auto &[key, value] : package.buildOptions)
                {
                    stream << key << " = " << toml::value<std::string>(value) << "\n";
                }
            }
            stream << "\n[package.artifacts]\n";
            stream << "mode = " << toml::value<std::string>(package.artifacts.mode) << "\n";
            LockData::writeStrings(stream, "include_dirs", package.artifacts.includeDirs);
            LockData::writeStrings(stream, "lib_dirs", package.artifacts.libDirs);
            LockData::writeStrings(stream, "bin_dirs", package.artifacts.binDirs);
            LockData::writeStrings(stream, "bin_files", package.artifacts.binFiles);
            LockData::writeStrings(stream, "libs", package.artifacts.libs);
            LockData::writeStrings(stream, "lib_files", package.artifacts.libFiles);
            LockData::writeStrings(stream, "defines", package.artifacts.defines);
            LockData::writeStrings(stream, "system_libs", package.artifacts.systemLibs);
            LockData::writeStrings(stream, "frameworks", package.artifacts.frameworks);
            stream << "\n";
        }

        std::string error;
        if (!File::write(path, stream.str(), &error))
        {
            diagnostics.error("failed to write lockfile: " + error);
            return false;
        }
        return true;
    }

    LockedPackage LockedPackage::from(const DependencyDesc &dependency, const std::string &commit,
                                      const std::string &config, const std::string &toolchainId)
    {
        return from(dependency, commit, config, toolchainId, {});
    }

    LockedPackage LockedPackage::from(const DependencyDesc &dependency, const std::string &commit,
                                      const std::string &config, const std::string &toolchainId,
                                      const std::string &toolchainContentHash)
    {
        LockedPackage package;
        package.name = dependency.name;
        package.source = dependency.source;
        package.sourceType = dependency.sourceType;
        package.sha256 = dependency.sha256;
        package.resolvedArchiveHash = dependency.resolvedArchiveHash;
        package.ref = dependency.ref;
        package.commit = commit;
        package.subdir = dependency.subdir;
        package.stripComponents = dependency.stripComponents;
        package.patches = dependency.patches;
        package.patchHashes = dependency.patchHashes;
        package.buildType = dependency.buildType;
        package.linkage = dependency.linkage;
        package.runtime = dependency.runtime;
        package.pic = dependency.pic;
        package.config = BuildConfig::normalize(config);
        package.toolchainId = toolchainId;
        package.buildHash = LockData::hash(dependency, commit, package.config, toolchainId, toolchainContentHash);
        package.artifactId = dependency.name + "/" + commit + "-" + package.buildHash;
        package.buildOptions = dependency.buildOptions;
        package.artifacts = dependency.artifacts;
        package.overridden = dependency.overridden;
        package.overridePath = dependency.overridePath;
        package.overrideSource = dependency.overrideSource;
        package.originalSource = dependency.originalSource;
        package.originalSourceType = dependency.originalSourceType;
        package.originalRef = dependency.originalRef;
        package.exportDefines = dependency.exportDefines;
        if (dependency.overridden)
        {
            package.artifactId = dependency.name + "/override-" + package.buildHash;
        }
        return package;
    }

    LockedPackage LockedPackage::from(const DependencyDesc &dependency, const std::string &commit)
    {
        return from(dependency, commit, "debug", {});
    }

} // namespace rhodium
