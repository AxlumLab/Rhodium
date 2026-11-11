/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/model/BuildModel.hpp"

#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <set>
#include <string>

namespace rhodium
{
    namespace
    {
        bool isBlank(const std::string &value)
        {
            return std::all_of(value.begin(), value.end(), [](unsigned char c)
                               { return std::isspace(c) != 0; });
        }

        bool isOneOf(const std::string &value, std::initializer_list<const char *> allowed)
        {
            const auto normalized = StringUtils::toLower(value);
            for (const char *item : allowed)
            {
                if (normalized == item)
                {
                    return true;
                }
            }
            return false;
        }

        void validateStringList(const std::vector<std::string> &values,
                                const std::string &field,
                                DiagnosticSink &diagnostics,
                                bool &ok)
        {
            for (const auto &value : values)
            {
                if (isBlank(value))
                {
                    diagnostics.error(field + " contains an empty path/value");
                    ok = false;
                }
            }
        }
    }

    bool validateBuildModel(const BuildModel &model, DiagnosticSink &diagnostics)
    {
        bool ok = true;

        if (isBlank(model.rootPackage.name))
        {
            diagnostics.error("package name is empty");
            ok = false;
        }

        std::set<std::string> dependencyNames;
        for (const auto &dependency : model.rootPackage.dependencies)
        {
            if (isBlank(dependency.name))
            {
                diagnostics.error("require dependency is missing name");
                ok = false;
            }
            else if (!dependencyNames.insert(dependency.name).second)
            {
                diagnostics.error("duplicate dependency name: " + dependency.name);
                ok = false;
            }

            if (dependency.sourceType == "git" && dependency.buildType != "header_only" && isBlank(dependency.ref))
            {
                diagnostics.error("require dependency " + dependency.source + " is missing ref");
                ok = false;
            }
            if (isBlank(dependency.source))
            {
                diagnostics.error("dependency source/path is empty");
                ok = false;
            }
            if (!isOneOf(dependency.sourceType, {"git", "local", "archive"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid source type " + dependency.sourceType);
                ok = false;
            }
            if (dependency.stripComponents < 0)
            {
                diagnostics.error("dependency " + dependency.name + " has invalid strip_components");
                ok = false;
            }
            if (!isOneOf(dependency.buildType, {"cmake", "make", "configure_make", "custom", "header_only", "prebuilt"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid build type " + dependency.buildType);
                ok = false;
            }
            if (!isOneOf(dependency.linkage, {"default", "static", "shared"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid linkage " + dependency.linkage);
                ok = false;
            }
            if (!isOneOf(dependency.runtime, {"default", "static", "dynamic"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid runtime " + dependency.runtime);
                ok = false;
            }
            if (!isOneOf(dependency.pic, {"default", "true", "false"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid pic " + dependency.pic);
                ok = false;
            }
            if (!isOneOf(dependency.artifacts.mode, {"install", "copy", "export"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid artifact mode " + dependency.artifacts.mode);
                ok = false;
            }
            validateStringList(dependency.artifacts.includeDirs, "dependency " + dependency.name + " artifact includes", diagnostics, ok);
            validateStringList(dependency.artifacts.libDirs, "dependency " + dependency.name + " artifact lib_dirs", diagnostics, ok);
            validateStringList(dependency.artifacts.binDirs, "dependency " + dependency.name + " artifact bin_dirs", diagnostics, ok);
            validateStringList(dependency.artifacts.binFiles, "dependency " + dependency.name + " artifact bin_files", diagnostics, ok);
            validateStringList(dependency.artifacts.libs, "dependency " + dependency.name + " artifact libs", diagnostics, ok);
            validateStringList(dependency.artifacts.libFiles, "dependency " + dependency.name + " artifact lib_files", diagnostics, ok);
            for (const auto &[config, files] : dependency.artifacts.libFilesByConfig)
            {
                if (!isOneOf(config, {"debug", "release", "all"}))
                {
                    diagnostics.error("dependency " + dependency.name + " artifact lib_files has invalid config " + config);
                    ok = false;
                }
                validateStringList(files, "dependency " + dependency.name + " artifact lib_files." + config, diagnostics, ok);
            }
        }

        return ok;
    }

} // namespace rhodium
