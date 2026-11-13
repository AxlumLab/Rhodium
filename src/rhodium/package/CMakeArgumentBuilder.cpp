/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/package/CMakeArgumentBuilder.hpp"

#include "rhodium/core/StringUtil.hpp"
#include "rhodium/package/BuildVariableExpander.hpp"

namespace rhodium
{
    std::string CMakeArgs::runtime(const std::string &value)
    {
        const auto normalized = StringUtils::toLower(value);
        if (normalized == "static")
        {
            return "MultiThreaded$<$<CONFIG:Debug>:Debug>";
        }
        if (normalized == "dynamic")
        {
            return "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL";
        }
        return {};
    }

    std::string CMakeArgs::boolOpt(const std::string &value)
    {
        const auto normalized = StringUtils::toLower(value);
        if (normalized == "true" || normalized == "on" || normalized == "yes")
        {
            return "ON";
        }
        if (normalized == "false" || normalized == "off" || normalized == "no")
        {
            return "OFF";
        }
        return {};
    }

    void CMakeArgs::setDefault(std::map<std::string, std::string> &options,
                               const std::string &key,
                               const std::string &value)
    {
        if (!value.empty() && options.find(key) == options.end())
        {
            options[key] = value;
        }
    }

    std::map<std::string, std::string> CMakeArgs::options(const DependencyDesc &dependency)
    {
        std::map<std::string, std::string> options;
        const auto linkage = StringUtils::toLower(dependency.linkage);
        if (linkage == "static")
        {
            options["BUILD_SHARED_LIBS"] = "OFF";
        }
        else if (linkage == "shared")
        {
            options["BUILD_SHARED_LIBS"] = "ON";
        }
        setDefault(options, "CMAKE_MSVC_RUNTIME_LIBRARY", runtime(dependency.runtime));
        setDefault(options, "CMAKE_POSITION_INDEPENDENT_CODE", boolOpt(dependency.pic));

        for (const auto &[key, value] : dependency.cmake.options)
        {
            options[key] = value;
        }
        for (const auto &[key, value] : dependency.buildOptions)
        {
            options[key] = value;
        }
        return options;
    }

    std::vector<std::string> CMakeArgs::configure(const DependencyDesc &dependency, const BuildUtil::Context &context)
    {
        std::vector<std::string> args = {
            "-S", context.sourceDir.string(),
            "-B", context.buildDir.string(),
            "-G", dependency.cmake.generator.empty() ? "Ninja" : dependency.cmake.generator,
            "-DCMAKE_INSTALL_PREFIX=" + context.artifactDir.string(),
        };

        const auto buildType = dependency.cmake.buildType.empty() ? context.config : dependency.cmake.buildType;
        if (!buildType.empty())
        {
            args.push_back("-DCMAKE_BUILD_TYPE=" + buildType);
        }
        if (!dependency.cmake.toolchainFile.empty())
        {
            args.push_back("-DCMAKE_TOOLCHAIN_FILE=" + BuildVar::expand(dependency.cmake.toolchainFile, context));
        }
        for (const auto &[key, value] : options(dependency))
        {
            args.push_back("-D" + key + "=" + BuildVar::expand(value, context));
        }
        auto extraArgs = BuildVar::expand(dependency.cmake.configureArgs, context);
        args.insert(args.end(), extraArgs.begin(), extraArgs.end());
        return args;
    }

    std::vector<std::string> CMakeArgs::wrap(const std::filesystem::path &workingDir,
                                             const std::map<std::string, std::string> &env,
                                             const std::string &executable,
                                             const std::vector<std::string> &args)
    {
        std::vector<std::string> wrapped = {"-E", "chdir", workingDir.string()};
        if (!env.empty())
        {
            wrapped.push_back("cmake");
            wrapped.push_back("-E");
            wrapped.push_back("env");
            for (const auto &[key, value] : env)
            {
                wrapped.push_back(key + "=" + value);
            }
        }
        wrapped.push_back(executable);
        wrapped.insert(wrapped.end(), args.begin(), args.end());
        return wrapped;
    }

} // namespace rhodium
