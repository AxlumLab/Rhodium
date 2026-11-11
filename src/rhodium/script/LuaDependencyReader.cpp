/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/script/LuaDependencyReader.hpp"

#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/script/LuaOverlayReader.hpp"
#include "rhodium/script/LuaValueReader.hpp"

#include <filesystem>
#include <utility>

namespace rhodium
{
    bool hasArchiveExtension(const std::string &source)
    {
        auto lower = StringUtils::toLower(source);
        const auto query = lower.find_first_of("?#");
        if (query != std::string::npos)
        {
            lower = lower.substr(0, query);
        }
        for (const auto &suffix : {".zip", ".tar", ".tar.gz", ".tgz", ".tar.xz", ".txz", ".tar.bz2", ".tbz2", ".7z"})
        {
            const std::string suffixText = suffix;
            if (lower.size() >= suffixText.size() && lower.substr(lower.size() - suffixText.size()) == suffixText)
            {
                return true;
            }
        }
        return false;
    }

    bool looksLikeGitSource(const std::string &source)
    {
        const auto lower = StringUtils::toLower(source);
        return lower.rfind("git@", 0) == 0 ||
               lower.rfind("ssh://", 0) == 0 ||
               lower.rfind("git://", 0) == 0 ||
               (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".git") ||
               lower.rfind("github.com/", 0) == 0 ||
               lower.rfind("https://github.com/", 0) == 0;
    }

    std::string inferSourceType(const std::string &source, sol::table options, bool local, const std::filesystem::path &workspaceRoot)
    {
        const auto explicitType = getStringOr(options, "source_type");
        if (!explicitType.empty())
        {
            return StringUtils::toLower(explicitType);
        }
        if (local)
        {
            return "local";
        }
        if (hasArchiveExtension(source))
        {
            return "archive";
        }
        if (looksLikeGitSource(source))
        {
            return "git";
        }
        auto path = std::filesystem::path(source);
        if (path.is_relative())
        {
            path = workspaceRoot / path;
        }
        if (std::filesystem::is_directory(path))
        {
            return "local";
        }
        return "git";
    }

    std::string inferDependencyName(const std::string &source)
    {
        auto text = source;
        while (!text.empty() && (text.back() == '/' || text.back() == '\\'))
        {
            text.pop_back();
        }
        auto slash = text.find_last_of("/\\:");
        auto name = slash == std::string::npos ? text : text.substr(slash + 1);
        if (name.size() > 4 && name.substr(name.size() - 4) == ".git")
        {
            name.resize(name.size() - 4);
        }
        return name;
    }

    void readBuildOptions(sol::table options, DependencyDesc &dependency)
    {
        sol::object cmakeValue = options["cmake"];
        if (!cmakeValue.is<sol::table>())
        {
            return;
        }
        sol::table cmake = cmakeValue.as<sol::table>();
        dependency.cmake.generator = getStringOr(cmake, "generator", dependency.cmake.generator);
        dependency.cmake.buildType = getStringOr(cmake, "build_type", dependency.cmake.buildType);
        dependency.cmake.configureArgs = getStringArrayOr(cmake, "configure_args");
        dependency.cmake.buildArgs = getStringArrayOr(cmake, "build_args");
        dependency.cmake.buildTargets = getStringArrayOr(cmake, "build_targets");
        dependency.cmake.installArgs = getStringArrayOr(cmake, "install_args");
        dependency.cmake.env = getStringMapOr(cmake, "env");
        dependency.cmake.toolchainFile = getStringOr(cmake, "toolchain_file", dependency.cmake.toolchainFile);
        dependency.cmake.install = getBoolOr(cmake, "install", dependency.cmake.install);
        dependency.cmake.installTarget = getStringOr(cmake, "install_target", dependency.cmake.installTarget);

        sol::object optionsValue = cmake["options"];
        if (optionsValue.is<sol::table>())
        {
            sol::table optionTable = optionsValue.as<sol::table>();
            for (const auto &item : optionTable)
            {
                if (item.first.is<std::string>())
                {
                    dependency.cmake.options[item.first.as<std::string>()] = objectToString(item.second);
                }
            }
        }
        dependency.buildOptions = dependency.cmake.options;
    }

    void readMakeOptions(sol::table options, DependencyDesc &dependency)
    {
        sol::object makeValue = options["make"];
        if (!makeValue.is<sol::table>())
        {
            return;
        }
        sol::table make = makeValue.as<sol::table>();
        dependency.make.executable = getStringOr(make, "executable", dependency.make.executable);
        dependency.make.jobs = getIntOr(make, "jobs", dependency.make.jobs);
        dependency.make.args = getStringArrayOr(make, "args");
        dependency.make.targets = getStringArrayOr(make, "targets");
        dependency.make.installTargets = getStringArrayOr(make, "install_targets");
        dependency.make.installArgs = getStringArrayOr(make, "install_args");
        dependency.make.env = getStringMapOr(make, "env");
    }

    void readConfigureOptions(sol::table options, DependencyDesc &dependency)
    {
        sol::object configureValue = options["configure"];
        if (!configureValue.is<sol::table>())
        {
            return;
        }
        sol::table configure = configureValue.as<sol::table>();
        dependency.configure.script = getStringOr(configure, "script", dependency.configure.script);
        dependency.configure.args = getStringArrayOr(configure, "args");
        dependency.configure.env = getStringMapOr(configure, "env");
    }

    void readCustomCommands(sol::table options, DependencyDesc &dependency)
    {
        sol::object commandsValue = options["commands"];
        if (!commandsValue.is<sol::table>())
        {
            return;
        }
        sol::table commands = commandsValue.as<sol::table>();
        dependency.commands.configure = getStringArrayOr(commands, "configure");
        dependency.commands.build = getStringArrayOr(commands, "build");
        dependency.commands.install = getStringArrayOr(commands, "install");
    }

    void readArtifacts(sol::table options, DependencyDesc &dependency)
    {
        sol::object artifactsValue = options["artifacts"];
        if (!artifactsValue.is<sol::table>())
        {
            return;
        }
        sol::table artifacts = artifactsValue.as<sol::table>();
        dependency.artifacts.mode = getStringOr(artifacts, "mode", dependency.artifacts.mode);
        dependency.artifacts.includeDirs = getStringArrayOr(artifacts, hasField(artifacts, "include_dirs") ? "include_dirs" : "includes");
        dependency.artifacts.libDirs = getStringArrayOr(artifacts, "lib_dirs");
        dependency.artifacts.binDirs = getStringArrayOr(artifacts, "bin_dirs");
        dependency.artifacts.binFiles = getStringArrayOr(artifacts, "bin_files");
        dependency.artifacts.libs = getStringArrayOr(artifacts, "libs");
        setStringArrayOrMapIfPresent(artifacts, "lib_files", dependency.artifacts.libFiles, dependency.artifacts.libFilesByConfig);
        dependency.artifacts.defines = getStringArrayOr(artifacts, "defines");
        dependency.artifacts.systemLibs = getStringArrayOr(artifacts, "system_libs");
        dependency.artifacts.frameworks = getStringArrayOr(artifacts, "frameworks");
    }

    void readToolchainRequirements(sol::table options, DependencyDesc &dependency)
    {
        sol::object requirementsValue = options["toolchain_requirements"];
        if (!requirementsValue.is<sol::table>())
        {
            return;
        }
        sol::table requirements = requirementsValue.as<sol::table>();
        dependency.toolchainRequirements.family = StringUtils::toLower(getStringOr(requirements, "family"));
        dependency.toolchainRequirements.abi = StringUtils::toLower(getStringOr(requirements, "abi"));
        dependency.toolchainRequirements.binaryFormat = StringUtils::toLower(getStringOr(requirements, "binary_format"));
    }

    void overlayCustomCommands(sol::table options, DependencyDesc &dependency)
    {
        sol::object commandsValue = options["commands"];
        if (!commandsValue.is<sol::table>())
        {
            return;
        }
        sol::table commands = commandsValue.as<sol::table>();
        setStringArrayIfPresent(commands, "configure", dependency.commands.configure);
        setStringArrayIfPresent(commands, "build", dependency.commands.build);
        setStringArrayIfPresent(commands, "install", dependency.commands.install);
    }

    void overlayArtifacts(sol::table options, DependencyDesc &dependency)
    {
        sol::object artifactsValue = options["artifacts"];
        if (!artifactsValue.is<sol::table>())
        {
            return;
        }
        sol::table artifacts = artifactsValue.as<sol::table>();
        setStringIfPresent(artifacts, "mode", dependency.artifacts.mode);
        if (hasField(artifacts, "include_dirs"))
        {
            setStringArrayIfPresent(artifacts, "include_dirs", dependency.artifacts.includeDirs);
        }
        else
        {
            setStringArrayIfPresent(artifacts, "includes", dependency.artifacts.includeDirs);
        }
        setStringArrayIfPresent(artifacts, "lib_dirs", dependency.artifacts.libDirs);
        setStringArrayIfPresent(artifacts, "bin_dirs", dependency.artifacts.binDirs);
        setStringArrayIfPresent(artifacts, "bin_files", dependency.artifacts.binFiles);
        setStringArrayIfPresent(artifacts, "libs", dependency.artifacts.libs);
        setStringArrayOrMapIfPresent(artifacts, "lib_files", dependency.artifacts.libFiles, dependency.artifacts.libFilesByConfig);
        setStringArrayIfPresent(artifacts, "defines", dependency.artifacts.defines);
        setStringArrayIfPresent(artifacts, "system_libs", dependency.artifacts.systemLibs);
        setStringArrayIfPresent(artifacts, "frameworks", dependency.artifacts.frameworks);
    }

    void overlayToolchainRequirements(sol::table options, DependencyDesc &dependency)
    {
        sol::object requirementsValue = options["toolchain_requirements"];
        if (!requirementsValue.is<sol::table>())
        {
            return;
        }
        sol::table requirements = requirementsValue.as<sol::table>();
        if (hasField(requirements, "family"))
        {
            dependency.toolchainRequirements.family = StringUtils::toLower(getStringOr(requirements, "family"));
        }
        if (hasField(requirements, "abi"))
        {
            dependency.toolchainRequirements.abi = StringUtils::toLower(getStringOr(requirements, "abi"));
        }
        if (hasField(requirements, "binary_format"))
        {
            dependency.toolchainRequirements.binaryFormat = StringUtils::toLower(getStringOr(requirements, "binary_format"));
        }
    }

    void overlayDependencyOptions(sol::table options, DependencyDesc &dependency)
    {
        setStringIfPresent(options, "name", dependency.name);
        setStringIfPresent(options, "source", dependency.source);
        setStringIfPresent(options, "ref", dependency.ref);
        setStringIfPresent(options, "subdir", dependency.subdir);
        setStringIfPresent(options, "build", dependency.buildType);
        setStringIfPresent(options, "linkage", dependency.linkage);
        setStringIfPresent(options, "runtime", dependency.runtime);
        setBoolStringIfPresent(options, "pic", dependency.pic);
        readBuildOptions(options, dependency);
        readMakeOptions(options, dependency);
        readConfigureOptions(options, dependency);
        overlayCustomCommands(options, dependency);
        overlayArtifacts(options, dependency);
        overlayToolchainRequirements(options, dependency);
    }

    DependencyDesc dependencyFromOptions(const std::string &idOrSource,
                                         sol::table options,
                                         bool local,
                                         const std::string &activePlatform,
                                         const std::filesystem::path &workspaceRoot)
    {
        DependencyDesc dep;
        dep.name = getStringOr(options, "name");

        if (local)
        {
            if (dep.name.empty())
            {
                dep.name = idOrSource;
            }
            dep.source = getStringOr(options, "path");
            if (dep.source.empty())
            {
                dep.source = idOrSource;
            }
        }
        else
        {
            dep.source = getStringOr(options, "source", idOrSource);
            if (dep.name.empty())
            {
                dep.name = hasField(options, "source") ? idOrSource : inferDependencyName(dep.source);
            }
        }

        dep.sourceType = inferSourceType(dep.source, options, local, workspaceRoot);
        dep.mirrors = getStringArrayOr(options, "mirrors");
        dep.sha256 = StringUtils::toLower(getStringOr(options, "sha256"));
        dep.ref = getStringOr(options, "ref");
        dep.subdir = getStringOr(options, "subdir", ".");
        dep.stripComponents = getIntOr(options, "strip_components", 0);
        dep.patches = getStringArrayOr(options, "patches");
        dep.buildType = getStringOr(options, "build", "header_only");
        dep.linkage = getStringOr(options, "linkage", "default");
        dep.runtime = getStringOr(options, "runtime", "default");
        dep.pic = getStringOr(options, "pic", "default");
        if (sol::object picValue = options["pic"]; picValue.is<bool>())
        {
            dep.pic = picValue.as<bool>() ? "true" : "false";
        }
        dep.local = dep.sourceType == "local";
        dep.dependencies = getStringArrayOr(options, "dependencies");
        dep.exportDefines = getBoolOr(options, "export_defines", false);
        readBuildOptions(options, dep);
        readMakeOptions(options, dep);
        readConfigureOptions(options, dep);
        readCustomCommands(options, dep);
        readArtifacts(options, dep);
        readToolchainRequirements(options, dep);
        applyDependencyOverlays(options, dep, activePlatform);
        return dep;
    }

    void readDependencyOverrides(sol::table table, BuildModel &model, DiagnosticSink &diagnostics)
    {
        for (const auto &item : table)
        {
            if (!item.first.is<std::string>())
            {
                diagnostics.error("dependency_overrides keys must be dependency names");
                continue;
            }

            DependencyOverrideDesc override;
            override.name = item.first.as<std::string>();
            if (item.second.is<std::string>())
            {
                override.path = item.second.as<std::string>();
            }
            else if (item.second.is<sol::table>())
            {
                override.path = getStringOr(item.second.as<sol::table>(), "path");
            }
            else
            {
                diagnostics.error("dependency override value must be a path string or table for " + override.name);
                continue;
            }

            if (override.path.empty())
            {
                diagnostics.error("dependency override path is empty for " + override.name);
                continue;
            }
            model.rootPackage.dependencyOverrides.push_back(std::move(override));
        }
    }

} // namespace rhodium
