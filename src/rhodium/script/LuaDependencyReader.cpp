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

    class DependencyOptions final
    {
    public:
        static bool isArchive(const std::string &source)
        {
            auto lower = StringUtils::toLower(source);
            const auto query = lower.find_first_of("?#");
            if (query != std::string::npos)
            {
                lower = lower.substr(0, query);
            }
            for (const auto &suffix :
                 {".zip", ".tar", ".tar.gz", ".tgz", ".tar.xz", ".txz", ".tar.bz2", ".tbz2", ".7z"})
            {
                const std::string suffixText = suffix;
                if (lower.size() >= suffixText.size() && lower.substr(lower.size() - suffixText.size()) == suffixText)
                {
                    return true;
                }
            }
            return false;
        }

        static bool isGit(const std::string &source)
        {
            const auto lower = StringUtils::toLower(source);
            return lower.rfind("git@", 0) == 0 || lower.rfind("ssh://", 0) == 0 || lower.rfind("git://", 0) == 0 ||
                   (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".git") ||
                   lower.rfind("github.com/", 0) == 0 || lower.rfind("https://github.com/", 0) == 0;
        }

        static std::string sourceType(const std::string &source, sol::table options, bool local,
                                      const std::filesystem::path &workspaceRoot)
        {
            const auto explicitType = LuaValueReader::string(options, "source_type");
            if (!explicitType.empty())
            {
                return StringUtils::toLower(explicitType);
            }
            if (local)
            {
                return "local";
            }
            if (DependencyOptions::isArchive(source))
            {
                return "archive";
            }
            if (DependencyOptions::isGit(source))
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

        static std::string name(const std::string &source)
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

        static void cmake(sol::table options, DependencyDesc &dependency)
        {
            sol::object cmakeValue = options["cmake"];
            if (!cmakeValue.is<sol::table>())
            {
                return;
            }
            sol::table cmake = cmakeValue.as<sol::table>();
            dependency.cmake.generator = LuaValueReader::string(cmake, "generator", dependency.cmake.generator);
            dependency.cmake.buildType = LuaValueReader::string(cmake, "build_type", dependency.cmake.buildType);
            dependency.cmake.configureArgs = LuaValueReader::array(cmake, "configure_args");
            dependency.cmake.buildArgs = LuaValueReader::array(cmake, "build_args");
            dependency.cmake.buildTargets = LuaValueReader::array(cmake, "build_targets");
            dependency.cmake.installArgs = LuaValueReader::array(cmake, "install_args");
            dependency.cmake.env = LuaValueReader::map(cmake, "env");
            dependency.cmake.toolchainFile =
                LuaValueReader::string(cmake, "toolchain_file", dependency.cmake.toolchainFile);
            dependency.cmake.install = LuaValueReader::boolean(cmake, "install", dependency.cmake.install);
            dependency.cmake.installTarget =
                LuaValueReader::string(cmake, "install_target", dependency.cmake.installTarget);

            sol::object optionsValue = cmake["options"];
            if (optionsValue.is<sol::table>())
            {
                sol::table optionTable = optionsValue.as<sol::table>();
                for (const auto &item : optionTable)
                {
                    if (item.first.is<std::string>())
                    {
                        dependency.cmake.options[item.first.as<std::string>()] = LuaValueReader::text(item.second);
                    }
                }
            }
            dependency.buildOptions = dependency.cmake.options;
        }

        static void make(sol::table options, DependencyDesc &dependency)
        {
            sol::object makeValue = options["make"];
            if (!makeValue.is<sol::table>())
            {
                return;
            }
            sol::table make = makeValue.as<sol::table>();
            dependency.make.executable = LuaValueReader::string(make, "executable", dependency.make.executable);
            dependency.make.jobs = LuaValueReader::integer(make, "jobs", dependency.make.jobs);
            dependency.make.args = LuaValueReader::array(make, "args");
            dependency.make.targets = LuaValueReader::array(make, "targets");
            dependency.make.installTargets = LuaValueReader::array(make, "install_targets");
            dependency.make.installArgs = LuaValueReader::array(make, "install_args");
            dependency.make.env = LuaValueReader::map(make, "env");
        }

        static void configure(sol::table options, DependencyDesc &dependency)
        {
            sol::object configureValue = options["configure"];
            if (!configureValue.is<sol::table>())
            {
                return;
            }
            sol::table configure = configureValue.as<sol::table>();
            dependency.configure.script = LuaValueReader::string(configure, "script", dependency.configure.script);
            dependency.configure.args = LuaValueReader::array(configure, "args");
            dependency.configure.env = LuaValueReader::map(configure, "env");
        }

        static void commands(sol::table options, DependencyDesc &dependency)
        {
            sol::object commandsValue = options["commands"];
            if (!commandsValue.is<sol::table>())
            {
                return;
            }
            sol::table commands = commandsValue.as<sol::table>();
            dependency.commands.configure = LuaValueReader::array(commands, "configure");
            dependency.commands.build = LuaValueReader::array(commands, "build");
            dependency.commands.install = LuaValueReader::array(commands, "install");
        }

        static void artifacts(sol::table options, DependencyDesc &dependency)
        {
            sol::object artifactsValue = options["artifacts"];
            if (!artifactsValue.is<sol::table>())
            {
                return;
            }
            sol::table artifacts = artifactsValue.as<sol::table>();
            dependency.artifacts.mode = LuaValueReader::string(artifacts, "mode", dependency.artifacts.mode);
            dependency.artifacts.includeDirs = LuaValueReader::array(
                artifacts, LuaValueReader::has(artifacts, "include_dirs") ? "include_dirs" : "includes");
            dependency.artifacts.libDirs = LuaValueReader::array(artifacts, "lib_dirs");
            dependency.artifacts.binDirs = LuaValueReader::array(artifacts, "bin_dirs");
            dependency.artifacts.binFiles = LuaValueReader::array(artifacts, "bin_files");
            dependency.artifacts.libs = LuaValueReader::array(artifacts, "libs");
            LuaValueReader::setArrayOrMap(artifacts, "lib_files", dependency.artifacts.libFiles,
                                          dependency.artifacts.libFilesByConfig);
            dependency.artifacts.defines = LuaValueReader::array(artifacts, "defines");
            dependency.artifacts.systemLibs = LuaValueReader::array(artifacts, "system_libs");
            dependency.artifacts.frameworks = LuaValueReader::array(artifacts, "frameworks");
        }

        static void requirements(sol::table options, DependencyDesc &dependency)
        {
            sol::object requirementsValue = options["toolchain_requirements"];
            if (!requirementsValue.is<sol::table>())
            {
                return;
            }
            sol::table requirements = requirementsValue.as<sol::table>();
            dependency.toolchainRequirements.family =
                StringUtils::toLower(LuaValueReader::string(requirements, "family"));
            dependency.toolchainRequirements.abi = StringUtils::toLower(LuaValueReader::string(requirements, "abi"));
            dependency.toolchainRequirements.binaryFormat =
                StringUtils::toLower(LuaValueReader::string(requirements, "binary_format"));
        }

        static void overlayCommands(sol::table options, DependencyDesc &dependency)
        {
            sol::object commandsValue = options["commands"];
            if (!commandsValue.is<sol::table>())
            {
                return;
            }
            sol::table commands = commandsValue.as<sol::table>();
            LuaValueReader::setArray(commands, "configure", dependency.commands.configure);
            LuaValueReader::setArray(commands, "build", dependency.commands.build);
            LuaValueReader::setArray(commands, "install", dependency.commands.install);
        }

        static void overlayArtifacts(sol::table options, DependencyDesc &dependency)
        {
            sol::object artifactsValue = options["artifacts"];
            if (!artifactsValue.is<sol::table>())
            {
                return;
            }
            sol::table artifacts = artifactsValue.as<sol::table>();
            LuaValueReader::setString(artifacts, "mode", dependency.artifacts.mode);
            if (LuaValueReader::has(artifacts, "include_dirs"))
            {
                LuaValueReader::setArray(artifacts, "include_dirs", dependency.artifacts.includeDirs);
            }
            else
            {
                LuaValueReader::setArray(artifacts, "includes", dependency.artifacts.includeDirs);
            }
            LuaValueReader::setArray(artifacts, "lib_dirs", dependency.artifacts.libDirs);
            LuaValueReader::setArray(artifacts, "bin_dirs", dependency.artifacts.binDirs);
            LuaValueReader::setArray(artifacts, "bin_files", dependency.artifacts.binFiles);
            LuaValueReader::setArray(artifacts, "libs", dependency.artifacts.libs);
            LuaValueReader::setArrayOrMap(artifacts, "lib_files", dependency.artifacts.libFiles,
                                          dependency.artifacts.libFilesByConfig);
            LuaValueReader::setArray(artifacts, "defines", dependency.artifacts.defines);
            LuaValueReader::setArray(artifacts, "system_libs", dependency.artifacts.systemLibs);
            LuaValueReader::setArray(artifacts, "frameworks", dependency.artifacts.frameworks);
        }

        static void overlayRequirements(sol::table options, DependencyDesc &dependency)
        {
            sol::object requirementsValue = options["toolchain_requirements"];
            if (!requirementsValue.is<sol::table>())
            {
                return;
            }
            sol::table requirements = requirementsValue.as<sol::table>();
            if (LuaValueReader::has(requirements, "family"))
            {
                dependency.toolchainRequirements.family =
                    StringUtils::toLower(LuaValueReader::string(requirements, "family"));
            }
            if (LuaValueReader::has(requirements, "abi"))
            {
                dependency.toolchainRequirements.abi =
                    StringUtils::toLower(LuaValueReader::string(requirements, "abi"));
            }
            if (LuaValueReader::has(requirements, "binary_format"))
            {
                dependency.toolchainRequirements.binaryFormat =
                    StringUtils::toLower(LuaValueReader::string(requirements, "binary_format"));
            }
        }
    };

    void LuaDependencyReader::overlay(sol::table options, DependencyDesc &dependency)
    {
        LuaValueReader::setString(options, "name", dependency.name);
        LuaValueReader::setString(options, "source", dependency.source);
        LuaValueReader::setString(options, "ref", dependency.ref);
        LuaValueReader::setString(options, "subdir", dependency.subdir);
        LuaValueReader::setString(options, "build", dependency.buildType);
        LuaValueReader::setString(options, "linkage", dependency.linkage);
        LuaValueReader::setString(options, "runtime", dependency.runtime);
        LuaValueReader::setBoolString(options, "pic", dependency.pic);
        DependencyOptions::cmake(options, dependency);
        DependencyOptions::make(options, dependency);
        DependencyOptions::configure(options, dependency);
        DependencyOptions::overlayCommands(options, dependency);
        DependencyOptions::overlayArtifacts(options, dependency);
        DependencyOptions::overlayRequirements(options, dependency);
    }

    DependencyDesc LuaDependencyReader::read(const std::string &idOrSource, sol::table options, bool local) const
    {
        DependencyDesc dep;
        dep.name = LuaValueReader::string(options, "name");

        if (local)
        {
            if (dep.name.empty())
            {
                dep.name = idOrSource;
            }
            dep.source = LuaValueReader::string(options, "path");
            if (dep.source.empty())
            {
                dep.source = idOrSource;
            }
        }
        else
        {
            dep.source = LuaValueReader::string(options, "source", idOrSource);
            if (dep.name.empty())
            {
                dep.name = LuaValueReader::has(options, "source") ? idOrSource : DependencyOptions::name(dep.source);
            }
        }

        dep.sourceType = DependencyOptions::sourceType(dep.source, options, local, workspaceRoot);
        dep.mirrors = LuaValueReader::array(options, "mirrors");
        dep.sha256 = StringUtils::toLower(LuaValueReader::string(options, "sha256"));
        dep.ref = LuaValueReader::string(options, "ref");
        dep.subdir = LuaValueReader::string(options, "subdir", ".");
        dep.stripComponents = LuaValueReader::integer(options, "strip_components", 0);
        dep.patches = LuaValueReader::array(options, "patches");
        dep.buildType = LuaValueReader::string(options, "build", "header_only");
        dep.linkage = LuaValueReader::string(options, "linkage", "default");
        dep.runtime = LuaValueReader::string(options, "runtime", "default");
        dep.pic = LuaValueReader::string(options, "pic", "default");
        if (sol::object picValue = options["pic"]; picValue.is<bool>())
        {
            dep.pic = picValue.as<bool>() ? "true" : "false";
        }
        dep.local = dep.sourceType == "local";
        dep.dependencies = LuaValueReader::array(options, "dependencies");
        dep.exportDefines = LuaValueReader::boolean(options, "export_defines", false);
        DependencyOptions::cmake(options, dep);
        DependencyOptions::make(options, dep);
        DependencyOptions::configure(options, dep);
        DependencyOptions::commands(options, dep);
        DependencyOptions::artifacts(options, dep);
        DependencyOptions::requirements(options, dep);
        LuaOverlayReader::apply(options, dep, activePlatform);
        return dep;
    }

    void LuaDependencyReader::overrides(sol::table table, BuildModel &model, DiagnosticSink &diagnostics)
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
                override.path = LuaValueReader::string(item.second.as<sol::table>(), "path");
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
