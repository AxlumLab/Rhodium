/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/core/Path.hpp"

#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <toml++/toml.hpp>

namespace rhodium
{
    static void writePathArray(std::ostringstream &stream, const char *key, const std::vector<std::filesystem::path> &values)
    {
        stream << key << " = [";
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i != 0)
            {
                stream << ", ";
            }
            stream << toml::value<std::string>(values[i].string());
        }
        stream << "]\n";
    }

    static void writeStringArray(std::ostringstream &stream, const char *key, const std::vector<std::string> &values)
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

    static std::vector<std::filesystem::path> readPathArray(const toml::table &table, const char *key)
    {
        std::vector<std::filesystem::path> values;
        const auto *array = table[key].as_array();
        if (!array)
        {
            return values;
        }
        for (const auto &item : *array)
        {
            if (auto value = item.value<std::string>())
            {
                values.emplace_back(*value);
            }
        }
        return values;
    }

    static std::vector<std::string> readStringArray(const toml::table &table, const char *key)
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

    static std::string readString(const toml::table &table, const char *key)
    {
        return table[key].value_or("");
    }

    static std::filesystem::path readPath(const toml::table &table, const char *key)
    {
        return std::filesystem::path(readString(table, key));
    }

    static std::string envValue(const std::string &key)
    {
#ifdef _WIN32
        char *value = nullptr;
        std::size_t size = 0;
        if (_dupenv_s(&value, &size, key.c_str()) != 0 || !value)
        {
            return {};
        }
        std::string result(value);
        std::free(value);
        return result;
#else
        const char *value = std::getenv(key.c_str());
        return value ? std::string(value) : std::string{};
#endif
    }

    static std::filesystem::path homePath()
    {
#ifdef _WIN32
        auto home = envValue("USERPROFILE");
#else
        auto home = envValue("HOME");
#endif
        return home.empty() ? std::filesystem::path{} : std::filesystem::path(home);
    }

    static void replaceAll(std::string &value, const std::string &from, const std::string &to)
    {
        std::size_t pos = 0;
        while ((pos = value.find(from, pos)) != std::string::npos)
        {
            value.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    static std::string expandText(std::string value,
                                  const std::filesystem::path &workspaceRoot,
                                  std::vector<std::string> &missing)
    {
        std::size_t pos = 0;
        while ((pos = value.find("$env{", pos)) != std::string::npos)
        {
            const auto end = value.find('}', pos + 5);
            if (end == std::string::npos)
            {
                break;
            }
            const auto key = value.substr(pos + 5, end - (pos + 5));
            const auto env = envValue(key);
            if (env.empty())
            {
                missing.push_back("missing_env:" + key);
            }
            value.replace(pos, end - pos + 1, env);
            pos += env.size();
        }
        pos = 0;
        while ((pos = value.find("${env:", pos)) != std::string::npos)
        {
            const auto end = value.find('}', pos + 6);
            if (end == std::string::npos)
            {
                break;
            }
            const auto key = value.substr(pos + 6, end - (pos + 6));
            const auto env = envValue(key);
            if (env.empty())
            {
                missing.push_back("missing_env:" + key);
            }
            value.replace(pos, end - pos + 1, env);
            pos += env.size();
        }
        replaceAll(value, "$workspace", workspaceRoot.string());
        replaceAll(value, "$home", homePath().string());
        return value;
    }

    static void expandPath(std::filesystem::path &path,
                           const std::filesystem::path &workspaceRoot,
                           std::vector<std::string> &missing)
    {
        if (!path.empty())
        {
            path = std::filesystem::path(expandText(path.string(), workspaceRoot, missing));
        }
    }

    static void expandPathArray(std::vector<std::filesystem::path> &paths,
                                const std::filesystem::path &workspaceRoot,
                                std::vector<std::string> &missing)
    {
        for (auto &path : paths)
        {
            expandPath(path, workspaceRoot, missing);
        }
    }

    static void addMissing(std::vector<std::string> &missing, const std::string &value)
    {
        if (std::find(missing.begin(), missing.end(), value) == missing.end())
        {
            missing.push_back(value);
        }
    }

    static bool pathExists(const std::filesystem::path &path)
    {
        return !path.empty() && File::exists(path);
    }

    static bool anyDirectoryExists(const std::vector<std::filesystem::path> &paths)
    {
        return std::any_of(paths.begin(), paths.end(), [](const auto &path) {
            return File::dir(path);
        });
    }

    static bool hasRequiredMissing(const std::vector<std::string> &missing)
    {
        return std::any_of(missing.begin(), missing.end(), [](const auto &value) {
            return value.rfind("optional:", 0) != 0;
        });
    }

    static void normalizeManualProfile(ToolchainProfile &profile,
                                       const std::filesystem::path &workspaceRoot)
    {
        std::vector<std::string> missing;
        expandPath(profile.compiler, workspaceRoot, missing);
        expandPath(profile.cxxCompiler, workspaceRoot, missing);
        expandPath(profile.linker, workspaceRoot, missing);
        expandPath(profile.archiver, workspaceRoot, missing);
        expandPath(profile.resourceCompiler, workspaceRoot, missing);
        expandPath(profile.manifestTool, workspaceRoot, missing);
        expandPath(profile.cmake, workspaceRoot, missing);
        expandPath(profile.ninja, workspaceRoot, missing);
        expandPath(profile.git, workspaceRoot, missing);
        expandPath(profile.strip, workspaceRoot, missing);
        expandPath(profile.ranlib, workspaceRoot, missing);
        expandPath(profile.adb, workspaceRoot, missing);
        expandPath(profile.java, workspaceRoot, missing);
        expandPath(profile.javac, workspaceRoot, missing);
        expandPath(profile.gradle, workspaceRoot, missing);
        expandPath(profile.sdkRoot, workspaceRoot, missing);
        expandPath(profile.sysroot, workspaceRoot, missing);
        expandPath(profile.androidSdkRoot, workspaceRoot, missing);
        expandPath(profile.androidNdkRoot, workspaceRoot, missing);
        expandPath(profile.jdkRoot, workspaceRoot, missing);
        expandPathArray(profile.systemIncludeDirs, workspaceRoot, missing);
        expandPathArray(profile.systemLibDirs, workspaceRoot, missing);
        expandPathArray(profile.binaryDirs, workspaceRoot, missing);
        for (auto &[_, value] : profile.environment)
        {
            value = expandText(value, workspaceRoot, missing);
        }

        if (profile.id.empty())
        {
            addMissing(missing, "id");
        }
        if (profile.platform.empty())
        {
            addMissing(missing, "platform");
        }
        if (profile.compilerKind.empty())
        {
            addMissing(missing, "compiler_kind");
        }
        if (profile.compiler.empty() || !pathExists(profile.compiler))
        {
            addMissing(missing, "compiler");
        }
        if (profile.cxxCompiler.empty() || !pathExists(profile.cxxCompiler))
        {
            addMissing(missing, "cxx_compiler");
        }
        if (profile.linker.empty() || !pathExists(profile.linker))
        {
            addMissing(missing, "linker");
        }
        if (profile.archiver.empty() || !pathExists(profile.archiver))
        {
            addMissing(missing, "archiver");
        }
        if (profile.ninja.empty() || !pathExists(profile.ninja))
        {
            addMissing(missing, "ninja");
        }
        if (profile.git.empty() || !pathExists(profile.git))
        {
            addMissing(missing, "optional:git");
        }
        if ((profile.platform == "windows" || profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (!anyDirectoryExists(profile.systemIncludeDirs) || !anyDirectoryExists(profile.systemLibDirs)))
        {
            if (!anyDirectoryExists(profile.systemIncludeDirs))
            {
                addMissing(missing, "include_dirs");
            }
            if (!anyDirectoryExists(profile.systemLibDirs))
            {
                addMissing(missing, "lib_dirs");
            }
        }
        if (profile.platform == "android" || profile.sdkKind == "android-ndk")
        {
            if (profile.androidSdkRoot.empty() || !File::dir(profile.androidSdkRoot))
            {
                addMissing(missing, "android_sdk_root");
            }
            if (profile.androidNdkRoot.empty() || !File::dir(profile.androidNdkRoot))
            {
                addMissing(missing, "android_ndk_root");
            }
            if (profile.androidAbi.empty())
            {
                addMissing(missing, "android_abi");
            }
            if (profile.androidApi.empty())
            {
                addMissing(missing, "android_api");
            }
            if (profile.sysroot.empty() || !File::dir(profile.sysroot))
            {
                addMissing(missing, "sysroot");
            }
            if (profile.targetTriple.empty())
            {
                addMissing(missing, "target_triple");
            }
        }
        profile.missing = std::move(missing);
        profile.complete = !hasRequiredMissing(profile.missing);
    }

    bool writeToolchainProfiles(const std::filesystem::path &path, const std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        std::ostringstream stream;
        stream << "version = 1\n\n";
        for (const auto &profile : profiles)
        {
            stream << "[[toolchain]]\n";
            stream << "id = " << toml::value<std::string>(profile.id) << "\n";
            stream << "platform = " << toml::value<std::string>(profile.platform) << "\n";
            stream << "host_arch = " << toml::value<std::string>(profile.hostArch) << "\n";
            stream << "target_arch = " << toml::value<std::string>(profile.targetArch) << "\n";
            stream << "compiler_kind = " << toml::value<std::string>(profile.compilerKind) << "\n";
            stream << "sdk_kind = " << toml::value<std::string>(profile.sdkKind) << "\n";
            stream << "compiler = " << toml::value<std::string>(profile.compiler.string()) << "\n";
            stream << "cxx_compiler = " << toml::value<std::string>(profile.cxxCompiler.string()) << "\n";
            stream << "linker = " << toml::value<std::string>(profile.linker.string()) << "\n";
            stream << "archiver = " << toml::value<std::string>(profile.archiver.string()) << "\n";
            stream << "resource_compiler = " << toml::value<std::string>(profile.resourceCompiler.string()) << "\n";
            stream << "manifest_tool = " << toml::value<std::string>(profile.manifestTool.string()) << "\n";
            stream << "cmake = " << toml::value<std::string>(profile.cmake.string()) << "\n";
            stream << "ninja = " << toml::value<std::string>(profile.ninja.string()) << "\n";
            stream << "git = " << toml::value<std::string>(profile.git.string()) << "\n";
            stream << "strip = " << toml::value<std::string>(profile.strip.string()) << "\n";
            stream << "ranlib = " << toml::value<std::string>(profile.ranlib.string()) << "\n";
            stream << "libtool = " << toml::value<std::string>(profile.libtool.string()) << "\n";
            stream << "lipo = " << toml::value<std::string>(profile.lipo.string()) << "\n";
            stream << "codesign = " << toml::value<std::string>(profile.codesign.string()) << "\n";
            stream << "adb = " << toml::value<std::string>(profile.adb.string()) << "\n";
            stream << "java = " << toml::value<std::string>(profile.java.string()) << "\n";
            stream << "javac = " << toml::value<std::string>(profile.javac.string()) << "\n";
            stream << "gradle = " << toml::value<std::string>(profile.gradle.string()) << "\n";
            stream << "compiler_version = " << toml::value<std::string>(profile.compilerVersion) << "\n";
            stream << "sdk_root = " << toml::value<std::string>(profile.sdkRoot.string()) << "\n";
            stream << "sysroot = " << toml::value<std::string>(profile.sysroot.string()) << "\n";
            stream << "sdk_version = " << toml::value<std::string>(profile.sdkVersion) << "\n";
            stream << "android_sdk_root = " << toml::value<std::string>(profile.androidSdkRoot.string()) << "\n";
            stream << "android_ndk_root = " << toml::value<std::string>(profile.androidNdkRoot.string()) << "\n";
            stream << "jdk_root = " << toml::value<std::string>(profile.jdkRoot.string()) << "\n";
            stream << "android_api = " << toml::value<std::string>(profile.androidApi) << "\n";
            stream << "android_abi = " << toml::value<std::string>(profile.androidAbi) << "\n";
            stream << "target_triple = " << toml::value<std::string>(profile.targetTriple) << "\n";
            stream << "deployment_target = " << toml::value<std::string>(profile.deploymentTarget) << "\n";
            stream << "complete = " << (profile.complete ? "true" : "false") << "\n";
            writeStringArray(stream, "missing", profile.missing);
            writePathArray(stream, "system_include_dirs", profile.systemIncludeDirs);
            writePathArray(stream, "system_lib_dirs", profile.systemLibDirs);
            writePathArray(stream, "binary_dirs", profile.binaryDirs);
            if (!profile.environment.empty())
            {
                stream << "\n[toolchain.environment]\n";
                for (const auto &[key, value] : profile.environment)
                {
                    stream << key << " = " << toml::value<std::string>(value) << "\n";
                }
            }
            stream << "\n";
        }

        std::string error;
        if (!File::write(path, stream.str(), &error))
        {
            diagnostics.error("failed to write toolchain cache: " + error);
            return false;
        }
        return true;
    }

    bool readToolchainProfiles(const std::filesystem::path &path, std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        profiles.clear();
        if (!File::exists(path))
        {
            return true;
        }

        try
        {
            auto table = toml::parse_file(path.string());
            auto toolchains = table["toolchain"].as_array();
            if (!toolchains)
            {
                return true;
            }

            for (const auto &node : *toolchains)
            {
                const auto *toolchain = node.as_table();
                if (!toolchain)
                {
                    continue;
                }

                ToolchainProfile profile;
                profile.id = readString(*toolchain, "id");
                profile.platform = readString(*toolchain, "platform");
                profile.hostArch = readString(*toolchain, "host_arch");
                profile.targetArch = readString(*toolchain, "target_arch");
                profile.compilerKind = readString(*toolchain, "compiler_kind");
                profile.sdkKind = readString(*toolchain, "sdk_kind");
                profile.compiler = readPath(*toolchain, "compiler");
                profile.cxxCompiler = readPath(*toolchain, "cxx_compiler");
                profile.linker = readPath(*toolchain, "linker");
                profile.archiver = readPath(*toolchain, "archiver");
                profile.resourceCompiler = readPath(*toolchain, "resource_compiler");
                profile.manifestTool = readPath(*toolchain, "manifest_tool");
                profile.cmake = readPath(*toolchain, "cmake");
                profile.ninja = readPath(*toolchain, "ninja");
                profile.git = readPath(*toolchain, "git");
                profile.strip = readPath(*toolchain, "strip");
                profile.ranlib = readPath(*toolchain, "ranlib");
                profile.libtool = readPath(*toolchain, "libtool");
                profile.lipo = readPath(*toolchain, "lipo");
                profile.codesign = readPath(*toolchain, "codesign");
                profile.adb = readPath(*toolchain, "adb");
                profile.java = readPath(*toolchain, "java");
                profile.javac = readPath(*toolchain, "javac");
                profile.gradle = readPath(*toolchain, "gradle");
                profile.compilerVersion = readString(*toolchain, "compiler_version");
                profile.sdkRoot = readPath(*toolchain, "sdk_root");
                profile.sysroot = readPath(*toolchain, "sysroot");
                if (profile.sysroot.empty())
                {
                    profile.sysroot = profile.sdkRoot;
                }
                profile.sdkVersion = readString(*toolchain, "sdk_version");
                profile.androidSdkRoot = readPath(*toolchain, "android_sdk_root");
                profile.androidNdkRoot = readPath(*toolchain, "android_ndk_root");
                profile.jdkRoot = readPath(*toolchain, "jdk_root");
                profile.androidApi = readString(*toolchain, "android_api");
                profile.androidAbi = readString(*toolchain, "android_abi");
                profile.targetTriple = readString(*toolchain, "target_triple");
                profile.deploymentTarget = readString(*toolchain, "deployment_target");
                profile.complete = (*toolchain)["complete"].value_or(false);
                profile.missing = readStringArray(*toolchain, "missing");
                profile.systemIncludeDirs = readPathArray(*toolchain, "system_include_dirs");
                profile.systemLibDirs = readPathArray(*toolchain, "system_lib_dirs");
                profile.binaryDirs = readPathArray(*toolchain, "binary_dirs");
                if (const auto *env = (*toolchain)["environment"].as_table())
                {
                    for (const auto &[key, value] : *env)
                    {
                        if (auto text = value.value<std::string>())
                        {
                            profile.environment[std::string(key.str())] = *text;
                        }
                    }
                }
                profiles.push_back(std::move(profile));
            }
            return true;
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to read toolchain cache: " + std::string(ex.what()));
            return false;
        }
    }

    std::filesystem::path localToolchainConfigPath(const std::filesystem::path &workspaceRoot)
    {
        return Path::stateRoot(workspaceRoot) / "local.toml";
    }

    bool readLocalToolchainConfig(const std::filesystem::path &workspaceRoot, LocalToolchainConfig &config, DiagnosticSink &diagnostics)
    {
        config = {};
        const auto path = localToolchainConfigPath(workspaceRoot);
        if (!File::exists(path))
        {
            return true;
        }

        try
        {
            auto table = toml::parse_file(path.string());
            config.preferredToolchain = table["preferred_toolchain"].value_or("");
            return true;
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to read local toolchain config: " + std::string(ex.what()));
            return false;
        }
    }

    bool writeLocalToolchainConfig(const std::filesystem::path &workspaceRoot, const LocalToolchainConfig &config, DiagnosticSink &diagnostics)
    {
        std::ostringstream stream;
        stream << "preferred_toolchain = " << toml::value<std::string>(config.preferredToolchain) << "\n";

        std::string error;
        if (!File::write(localToolchainConfigPath(workspaceRoot), stream.str(), &error))
        {
            diagnostics.error("failed to write local toolchain config: " + error);
            return false;
        }
        return true;
    }

    const char *toString(ToolchainSource source)
    {
        switch (source)
        {
        case ToolchainSource::AutoDetected:
            return "auto";
        case ToolchainSource::GlobalUser:
            return "global-user";
        case ToolchainSource::Workspace:
            return "workspace";
        case ToolchainSource::ProjectUser:
            return "project-user";
        }
        return "auto";
    }

    bool parseToolchainSource(const std::string &value, ToolchainSource &source)
    {
        if (value == "auto")
        {
            source = ToolchainSource::AutoDetected;
            return true;
        }
        if (value == "global-user" || value == "global")
        {
            source = ToolchainSource::GlobalUser;
            return true;
        }
        if (value == "workspace")
        {
            source = ToolchainSource::Workspace;
            return true;
        }
        if (value == "project-user" || value == "user")
        {
            source = ToolchainSource::ProjectUser;
            return true;
        }
        return false;
    }

    std::filesystem::path projectUserToolchainsPath(const std::filesystem::path &workspaceRoot)
    {
        return Path::stateRoot(workspaceRoot) / "toolchains" / "user.toml";
    }

    std::filesystem::path globalUserToolchainsPath()
    {
        return Path::stateRoot(homePath()) / "toolchains.toml";
    }

    static std::vector<std::pair<ToolchainSource, std::filesystem::path>> toolchainSourceFiles(const std::filesystem::path &workspaceRoot)
    {
        return {
            {ToolchainSource::AutoDetected, Path::stateRoot(workspaceRoot) / "toolchains" / "host.toml"},
            {ToolchainSource::GlobalUser, globalUserToolchainsPath()},
            {ToolchainSource::Workspace, workspaceRoot / "config" / "toolchains.toml"},
            {ToolchainSource::Workspace, workspaceRoot / "toolchains.toml"},
            {ToolchainSource::ProjectUser, projectUserToolchainsPath(workspaceRoot)},
        };
    }

    std::vector<LoadedToolchainProfile> loadToolchainSources(const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics)
    {
        std::vector<LoadedToolchainProfile> result;
        for (const auto &[source, path] : toolchainSourceFiles(workspaceRoot))
        {
            if (path.empty() || !File::exists(path))
            {
                continue;
            }
            std::vector<ToolchainProfile> profiles;
            if (!readToolchainProfiles(path, profiles, diagnostics))
            {
                return {};
            }
            for (auto &profile : profiles)
            {
                if (source != ToolchainSource::AutoDetected)
                {
                    normalizeManualProfile(profile, workspaceRoot);
                }
                LoadedToolchainProfile loaded;
                loaded.profile = std::move(profile);
                loaded.source = source;
                loaded.sourceFile = path;
                result.push_back(std::move(loaded));
            }
        }
        return result;
    }

    std::vector<LoadedToolchainProfile> loadMergedToolchains(const std::filesystem::path &workspaceRoot, DiagnosticSink &diagnostics)
    {
        const auto loaded = loadToolchainSources(workspaceRoot, diagnostics);
        std::vector<LoadedToolchainProfile> merged;
        std::map<std::string, std::size_t> byId;
        for (auto profile : loaded)
        {
            if (profile.profile.id.empty())
            {
                diagnostics.warning("toolchain entry without id ignored: " + profile.sourceFile.string());
                continue;
            }
            auto found = byId.find(profile.profile.id);
            if (found == byId.end())
            {
                byId[profile.profile.id] = merged.size();
                merged.push_back(std::move(profile));
                continue;
            }
            auto &slot = merged[found->second];
            profile.overridden = std::move(slot.overridden);
            profile.overridden.push_back(ToolchainOverrideRecord{slot.profile.id, slot.source, slot.sourceFile});
            slot = std::move(profile);
        }
        return merged;
    }

    std::vector<ToolchainProfile> unwrapToolchainProfiles(const std::vector<LoadedToolchainProfile> &profiles)
    {
        std::vector<ToolchainProfile> result;
        for (const auto &profile : profiles)
        {
            result.push_back(profile.profile);
        }
        return result;
    }

    static std::filesystem::path writableToolchainSourcePath(const std::filesystem::path &workspaceRoot, ToolchainSource source)
    {
        if (source == ToolchainSource::ProjectUser)
        {
            return projectUserToolchainsPath(workspaceRoot);
        }
        if (source == ToolchainSource::GlobalUser)
        {
            return globalUserToolchainsPath();
        }
        return {};
    }

    bool readUserToolchainProfiles(const std::filesystem::path &workspaceRoot, ToolchainSource source, std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        const auto path = writableToolchainSourcePath(workspaceRoot, source);
        if (path.empty())
        {
            diagnostics.error("toolchain source is not writable: " + std::string(toString(source)));
            return false;
        }
        return readToolchainProfiles(path, profiles, diagnostics);
    }

    bool writeUserToolchainProfiles(const std::filesystem::path &workspaceRoot, ToolchainSource source, const std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        const auto path = writableToolchainSourcePath(workspaceRoot, source);
        if (path.empty())
        {
            diagnostics.error("toolchain source is not writable: " + std::string(toString(source)));
            return false;
        }
        std::string error;
        if (!File::mkdir(path.parent_path(), &error))
        {
            diagnostics.error("failed to create toolchain directory: " + error);
            return false;
        }
        if (profiles.empty())
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
            if (ec)
            {
                diagnostics.error("failed to remove empty toolchain file: " + ec.message());
                return false;
            }
            return true;
        }
        return writeToolchainProfiles(path, profiles, diagnostics);
    }
}
