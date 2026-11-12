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
    namespace
    {
        class ProfileFields final
        {
        public:
            static ToolchainProfile read(const toml::table &table)
            {
                ToolchainProfile profile;
                profile.id = ProfileFields::string(table, "id");
                profile.platform = ProfileFields::string(table, "platform");
                profile.hostArch = ProfileFields::string(table, "host_arch");
                profile.targetArch = ProfileFields::string(table, "target_arch");
                profile.compilerKind = ProfileFields::string(table, "compiler_kind");
                profile.sdkKind = ProfileFields::string(table, "sdk_kind");
                profile.compiler = ProfileFields::path(table, "compiler");
                profile.cxxCompiler = ProfileFields::path(table, "cxx_compiler");
                profile.linker = ProfileFields::path(table, "linker");
                profile.archiver = ProfileFields::path(table, "archiver");
                profile.resourceCompiler = ProfileFields::path(table, "resource_compiler");
                profile.manifestTool = ProfileFields::path(table, "manifest_tool");
                profile.cmake = ProfileFields::path(table, "cmake");
                profile.ninja = ProfileFields::path(table, "ninja");
                profile.git = ProfileFields::path(table, "git");
                profile.strip = ProfileFields::path(table, "strip");
                profile.ranlib = ProfileFields::path(table, "ranlib");
                profile.libtool = ProfileFields::path(table, "libtool");
                profile.lipo = ProfileFields::path(table, "lipo");
                profile.codesign = ProfileFields::path(table, "codesign");
                profile.adb = ProfileFields::path(table, "adb");
                profile.java = ProfileFields::path(table, "java");
                profile.javac = ProfileFields::path(table, "javac");
                profile.gradle = ProfileFields::path(table, "gradle");
                profile.compilerVersion = ProfileFields::string(table, "compiler_version");
                profile.sdkRoot = ProfileFields::path(table, "sdk_root");
                profile.sysroot = ProfileFields::path(table, "sysroot");
                if (profile.sysroot.empty())
                {
                    profile.sysroot = profile.sdkRoot;
                }
                profile.sdkVersion = ProfileFields::string(table, "sdk_version");
                profile.androidSdkRoot = ProfileFields::path(table, "android_sdk_root");
                profile.androidNdkRoot = ProfileFields::path(table, "android_ndk_root");
                profile.jdkRoot = ProfileFields::path(table, "jdk_root");
                profile.androidApi = ProfileFields::string(table, "android_api");
                profile.androidAbi = ProfileFields::string(table, "android_abi");
                profile.targetTriple = ProfileFields::string(table, "target_triple");
                profile.deploymentTarget = ProfileFields::string(table, "deployment_target");
                profile.complete = (table)["complete"].value_or(false);
                profile.missing = ProfileFields::strings(table, "missing");
                profile.systemIncludeDirs = ProfileFields::paths(table, "system_include_dirs");
                profile.systemLibDirs = ProfileFields::paths(table, "system_lib_dirs");
                profile.binaryDirs = ProfileFields::paths(table, "binary_dirs");
                if (const auto *env = (table)["environment"].as_table())
                {
                    for (const auto &[key, value] : *env)
                    {
                        if (auto text = value.value<std::string>())
                        {
                            profile.environment[std::string(key.str())] = *text;
                        }
                    }
                }
                return profile;
            }

            static void writePaths(std::ostringstream &stream, const char *key,
                                   const std::vector<std::filesystem::path> &values)
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

            static void writeStrings(std::ostringstream &stream, const char *key,
                                     const std::vector<std::string> &values)
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

            static std::vector<std::filesystem::path> paths(const toml::table &table, const char *key)
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

            static std::string string(const toml::table &table, const char *key)
            {
                return table[key].value_or("");
            }

            static std::filesystem::path path(const toml::table &table, const char *key)
            {
                return std::filesystem::path(ProfileFields::string(table, key));
            }
        };
    }

    namespace
    {
        class ProfileEnvironment final
        {
        public:
            static std::string value(const std::string &key)
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

            static std::filesystem::path home()
            {
#ifdef _WIN32
                auto home = ProfileEnvironment::value("USERPROFILE");
#else
                auto home = ProfileEnvironment::value("HOME");
#endif
                return home.empty() ? std::filesystem::path{} : std::filesystem::path(home);
            }

            static void replace(std::string &value, const std::string &from, const std::string &to)
            {
                std::size_t pos = 0;
                while ((pos = value.find(from, pos)) != std::string::npos)
                {
                    value.replace(pos, from.size(), to);
                    pos += to.size();
                }
            }

            static std::string expand(std::string value, const std::filesystem::path &workspaceRoot,
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
                    const auto env = ProfileEnvironment::value(key);
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
                    const auto env = ProfileEnvironment::value(key);
                    if (env.empty())
                    {
                        missing.push_back("missing_env:" + key);
                    }
                    value.replace(pos, end - pos + 1, env);
                    pos += env.size();
                }
                ProfileEnvironment::replace(value, "$workspace", workspaceRoot.string());
                ProfileEnvironment::replace(value, "$home", ProfileEnvironment::home().string());
                return value;
            }

            static void path(std::filesystem::path &path, const std::filesystem::path &workspaceRoot,
                             std::vector<std::string> &missing)
            {
                if (!path.empty())
                {
                    path = std::filesystem::path(ProfileEnvironment::expand(path.string(), workspaceRoot, missing));
                }
            }

            static void paths(std::vector<std::filesystem::path> &paths, const std::filesystem::path &workspaceRoot,
                              std::vector<std::string> &missing)
            {
                for (auto &path : paths)
                {
                    ProfileEnvironment::path(path, workspaceRoot, missing);
                }
            }
        };
    }

    namespace
    {
        class ProfileValidation final
        {
        public:
            static void addMissing(std::vector<std::string> &missing, const std::string &value)
            {
                if (std::find(missing.begin(), missing.end(), value) == missing.end())
                {
                    missing.push_back(value);
                }
            }

            static bool exists(const std::filesystem::path &path)
            {
                return !path.empty() && File::exists(path);
            }

            static bool directories(const std::vector<std::filesystem::path> &paths)
            {
                return std::any_of(paths.begin(), paths.end(),
                                   [](const auto &path)
                                   {
                                       return File::dir(path);
                                   });
            }

            static bool hasRequiredMissing(const std::vector<std::string> &missing)
            {
                return std::any_of(missing.begin(), missing.end(),
                                   [](const auto &value)
                                   {
                                       return value.rfind("optional:", 0) != 0;
                                   });
            }

            static void expand(ToolchainProfile &profile, const std::filesystem::path &workspaceRoot,
                               std::vector<std::string> &missing)
            {
                ProfileEnvironment::path(profile.compiler, workspaceRoot, missing);
                ProfileEnvironment::path(profile.cxxCompiler, workspaceRoot, missing);
                ProfileEnvironment::path(profile.linker, workspaceRoot, missing);
                ProfileEnvironment::path(profile.archiver, workspaceRoot, missing);
                ProfileEnvironment::path(profile.resourceCompiler, workspaceRoot, missing);
                ProfileEnvironment::path(profile.manifestTool, workspaceRoot, missing);
                ProfileEnvironment::path(profile.cmake, workspaceRoot, missing);
                ProfileEnvironment::path(profile.ninja, workspaceRoot, missing);
                ProfileEnvironment::path(profile.git, workspaceRoot, missing);
                ProfileEnvironment::path(profile.strip, workspaceRoot, missing);
                ProfileEnvironment::path(profile.ranlib, workspaceRoot, missing);
                ProfileEnvironment::path(profile.adb, workspaceRoot, missing);
                ProfileEnvironment::path(profile.java, workspaceRoot, missing);
                ProfileEnvironment::path(profile.javac, workspaceRoot, missing);
                ProfileEnvironment::path(profile.gradle, workspaceRoot, missing);
                ProfileEnvironment::path(profile.sdkRoot, workspaceRoot, missing);
                ProfileEnvironment::path(profile.sysroot, workspaceRoot, missing);
                ProfileEnvironment::path(profile.androidSdkRoot, workspaceRoot, missing);
                ProfileEnvironment::path(profile.androidNdkRoot, workspaceRoot, missing);
                ProfileEnvironment::path(profile.jdkRoot, workspaceRoot, missing);
                ProfileEnvironment::paths(profile.systemIncludeDirs, workspaceRoot, missing);
                ProfileEnvironment::paths(profile.systemLibDirs, workspaceRoot, missing);
                ProfileEnvironment::paths(profile.binaryDirs, workspaceRoot, missing);
                for (auto &[_, value] : profile.environment)
                {
                    value = ProfileEnvironment::expand(value, workspaceRoot, missing);
                }
            }

            static void required(const ToolchainProfile &profile, std::vector<std::string> &missing)
            {
                if (profile.id.empty())
                {
                    ProfileValidation::addMissing(missing, "id");
                }
                if (profile.platform.empty())
                {
                    ProfileValidation::addMissing(missing, "platform");
                }
                if (profile.compilerKind.empty())
                {
                    ProfileValidation::addMissing(missing, "compiler_kind");
                }
                if (profile.compiler.empty() || !ProfileValidation::exists(profile.compiler))
                {
                    ProfileValidation::addMissing(missing, "compiler");
                }
                if (profile.cxxCompiler.empty() || !ProfileValidation::exists(profile.cxxCompiler))
                {
                    ProfileValidation::addMissing(missing, "cxx_compiler");
                }
                if (profile.linker.empty() || !ProfileValidation::exists(profile.linker))
                {
                    ProfileValidation::addMissing(missing, "linker");
                }
                if (profile.archiver.empty() || !ProfileValidation::exists(profile.archiver))
                {
                    ProfileValidation::addMissing(missing, "archiver");
                }
                if (profile.ninja.empty() || !ProfileValidation::exists(profile.ninja))
                {
                    ProfileValidation::addMissing(missing, "ninja");
                }
                if (profile.git.empty() || !ProfileValidation::exists(profile.git))
                {
                    ProfileValidation::addMissing(missing, "optional:git");
                }
                if ((profile.platform == "windows" || profile.compilerKind == "msvc" ||
                     profile.compilerKind == "clang-cl") &&
                    (!ProfileValidation::directories(profile.systemIncludeDirs) ||
                     !ProfileValidation::directories(profile.systemLibDirs)))
                {
                    if (!ProfileValidation::directories(profile.systemIncludeDirs))
                    {
                        ProfileValidation::addMissing(missing, "include_dirs");
                    }
                    if (!ProfileValidation::directories(profile.systemLibDirs))
                    {
                        ProfileValidation::addMissing(missing, "lib_dirs");
                    }
                }
            }

            static void normalize(ToolchainProfile &profile, const std::filesystem::path &workspaceRoot)
            {
                std::vector<std::string> missing;
                expand(profile, workspaceRoot, missing);

                required(profile, missing);
                if (profile.platform == "android" || profile.sdkKind == "android-ndk")
                {
                    if (profile.androidSdkRoot.empty() || !File::dir(profile.androidSdkRoot))
                    {
                        ProfileValidation::addMissing(missing, "android_sdk_root");
                    }
                    if (profile.androidNdkRoot.empty() || !File::dir(profile.androidNdkRoot))
                    {
                        ProfileValidation::addMissing(missing, "android_ndk_root");
                    }
                    if (profile.androidAbi.empty())
                    {
                        ProfileValidation::addMissing(missing, "android_abi");
                    }
                    if (profile.androidApi.empty())
                    {
                        ProfileValidation::addMissing(missing, "android_api");
                    }
                    if (profile.sysroot.empty() || !File::dir(profile.sysroot))
                    {
                        ProfileValidation::addMissing(missing, "sysroot");
                    }
                    if (profile.targetTriple.empty())
                    {
                        ProfileValidation::addMissing(missing, "target_triple");
                    }
                }
                profile.missing = std::move(missing);
                profile.complete = !ProfileValidation::hasRequiredMissing(profile.missing);
            }
        };
    }

    bool ToolchainRegistry::write(const std::filesystem::path &path, const std::vector<ToolchainProfile> &profiles,
                                  DiagnosticSink &diagnostics)
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
            ProfileFields::writeStrings(stream, "missing", profile.missing);
            ProfileFields::writePaths(stream, "system_include_dirs", profile.systemIncludeDirs);
            ProfileFields::writePaths(stream, "system_lib_dirs", profile.systemLibDirs);
            ProfileFields::writePaths(stream, "binary_dirs", profile.binaryDirs);
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

    bool ToolchainRegistry::read(const std::filesystem::path &path, std::vector<ToolchainProfile> &profiles,
                                 DiagnosticSink &diagnostics)
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

                profiles.push_back(ProfileFields::read(*toolchain));
            }
            return true;
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to read toolchain cache: " + std::string(ex.what()));
            return false;
        }
    }

    std::filesystem::path ToolchainRegistry::configPath(const std::filesystem::path &workspaceRoot)
    {
        return Path::stateRoot(workspaceRoot) / "local.toml";
    }

    bool ToolchainRegistry::readConfig(const std::filesystem::path &workspaceRoot, LocalToolchainConfig &config,
                                       DiagnosticSink &diagnostics)
    {
        config = {};
        const auto path = ToolchainRegistry::configPath(workspaceRoot);
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

    bool ToolchainRegistry::writeConfig(const std::filesystem::path &workspaceRoot, const LocalToolchainConfig &config,
                                        DiagnosticSink &diagnostics)
    {
        std::ostringstream stream;
        stream << "preferred_toolchain = " << toml::value<std::string>(config.preferredToolchain) << "\n";

        std::string error;
        if (!File::write(ToolchainRegistry::configPath(workspaceRoot), stream.str(), &error))
        {
            diagnostics.error("failed to write local toolchain config: " + error);
            return false;
        }
        return true;
    }

    const char *ToolchainRegistry::name(ToolchainSource source)
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

    bool ToolchainRegistry::parseSource(const std::string &value, ToolchainSource &source)
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

    std::filesystem::path ToolchainRegistry::projectPath(const std::filesystem::path &workspaceRoot)
    {
        return Path::stateRoot(workspaceRoot) / "toolchains" / "user.toml";
    }

    std::filesystem::path ToolchainRegistry::globalPath()
    {
        return Path::stateRoot(ProfileEnvironment::home()) / "toolchains.toml";
    }

    namespace
    {
        class RegistryPaths final
        {
        public:
            static std::vector<std::pair<ToolchainSource, std::filesystem::path>> sources(
                const std::filesystem::path &workspaceRoot)
            {
                return {
                    {ToolchainSource::AutoDetected, Path::stateRoot(workspaceRoot) / "toolchains" / "host.toml"},
                    {ToolchainSource::GlobalUser, ToolchainRegistry::globalPath()},
                    {ToolchainSource::Workspace, workspaceRoot / "config" / "toolchains.toml"},
                    {ToolchainSource::Workspace, workspaceRoot / "toolchains.toml"},
                    {ToolchainSource::ProjectUser, ToolchainRegistry::projectPath(workspaceRoot)},
                };
            }

            static std::filesystem::path writable(const std::filesystem::path &workspaceRoot, ToolchainSource source)
            {
                if (source == ToolchainSource::ProjectUser)
                {
                    return ToolchainRegistry::projectPath(workspaceRoot);
                }
                if (source == ToolchainSource::GlobalUser)
                {
                    return ToolchainRegistry::globalPath();
                }
                return {};
            }
        };
    }

    std::vector<LoadedToolchainProfile> ToolchainRegistry::sources(const std::filesystem::path &workspaceRoot,
                                                                   DiagnosticSink &diagnostics)
    {
        std::vector<LoadedToolchainProfile> result;
        for (const auto &[source, path] : RegistryPaths::sources(workspaceRoot))
        {
            if (path.empty() || !File::exists(path))
            {
                continue;
            }
            std::vector<ToolchainProfile> profiles;
            if (!ToolchainRegistry::read(path, profiles, diagnostics))
            {
                return {};
            }
            for (auto &profile : profiles)
            {
                if (source != ToolchainSource::AutoDetected)
                {
                    ProfileValidation::normalize(profile, workspaceRoot);
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

    std::vector<LoadedToolchainProfile> ToolchainRegistry::load(const std::filesystem::path &workspaceRoot,
                                                                DiagnosticSink &diagnostics)
    {
        const auto loaded = ToolchainRegistry::sources(workspaceRoot, diagnostics);
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

    std::vector<ToolchainProfile> ToolchainRegistry::profiles(const std::vector<LoadedToolchainProfile> &profiles)
    {
        std::vector<ToolchainProfile> result;
        for (const auto &profile : profiles)
        {
            result.push_back(profile.profile);
        }
        return result;
    }

    bool ToolchainRegistry::readUser(const std::filesystem::path &workspaceRoot, ToolchainSource source,
                                     std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        const auto path = RegistryPaths::writable(workspaceRoot, source);
        if (path.empty())
        {
            diagnostics.error("toolchain source is not writable: " + std::string(ToolchainRegistry::name(source)));
            return false;
        }
        return ToolchainRegistry::read(path, profiles, diagnostics);
    }

    bool ToolchainRegistry::writeUser(const std::filesystem::path &workspaceRoot, ToolchainSource source,
                                      const std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        const auto path = RegistryPaths::writable(workspaceRoot, source);
        if (path.empty())
        {
            diagnostics.error("toolchain source is not writable: " + std::string(ToolchainRegistry::name(source)));
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
        return ToolchainRegistry::write(path, profiles, diagnostics);
    }
}
