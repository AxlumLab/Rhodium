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

    class ModelFields final
    {
    public:
        static bool blank(const std::string &value)
        {
            return std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
        }

        static bool oneOf(const std::string &value, std::initializer_list<const char *> allowed)
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

        static void strings(const std::vector<std::string> &values, const std::string &field,
                            DiagnosticSink &diagnostics, bool &ok)
        {
            for (const auto &value : values)
            {
                if (ModelFields::blank(value))
                {
                    diagnostics.error(field + " contains an empty path/value");
                    ok = false;
                }
            }
        }
    };

    class DependencyValidation final
    {
    public:
        static bool validate(const DependencyDesc &dependency, std::set<std::string> &dependencyNames,
                             DiagnosticSink &diagnostics)
        {

            bool ok = true;
            if (ModelFields::blank(dependency.name))
            {
                diagnostics.error("require dependency is missing name");
                ok = false;
            }
            else if (!dependencyNames.insert(dependency.name).second)
            {
                diagnostics.error("duplicate dependency name: " + dependency.name);
                ok = false;
            }

            ok &= source(dependency, diagnostics);
            ok &= options(dependency, diagnostics);
            ok &= artifacts(dependency, diagnostics);
            return ok;
        }

        static bool source(const DependencyDesc &dependency, DiagnosticSink &diagnostics)
        {
            bool ok = true;
            if (dependency.sourceType == "git" && dependency.buildType != "header_only" &&
                ModelFields::blank(dependency.ref))
            {
                diagnostics.error("require dependency " + dependency.source + " is missing ref");
                ok = false;
            }
            if (ModelFields::blank(dependency.source))
            {
                diagnostics.error("dependency source/path is empty");
                ok = false;
            }
            if (!ModelFields::oneOf(dependency.sourceType, {"git", "local", "archive"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid source type " +
                                  dependency.sourceType);
                ok = false;
            }
            if (dependency.stripComponents < 0)
            {
                diagnostics.error("dependency " + dependency.name + " has invalid strip_components");
                ok = false;
            }

            return ok;
        }

        static bool options(const DependencyDesc &dependency, DiagnosticSink &diagnostics)
        {
            bool ok = true;
            if (!ModelFields::oneOf(dependency.buildType,
                                    {"cmake", "make", "configure_make", "custom", "header_only", "prebuilt"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid build type " + dependency.buildType);
                ok = false;
            }
            if (!ModelFields::oneOf(dependency.linkage, {"default", "static", "shared"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid linkage " + dependency.linkage);
                ok = false;
            }
            if (!ModelFields::oneOf(dependency.runtime, {"default", "static", "dynamic"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid runtime " + dependency.runtime);
                ok = false;
            }
            if (!ModelFields::oneOf(dependency.pic, {"default", "true", "false"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid pic " + dependency.pic);
                ok = false;
            }

            return ok;
        }

        static bool artifacts(const DependencyDesc &dependency, DiagnosticSink &diagnostics)
        {
            bool ok = true;
            if (!ModelFields::oneOf(dependency.artifacts.mode, {"install", "copy", "export"}))
            {
                diagnostics.error("dependency " + dependency.name + " has invalid artifact mode " +
                                  dependency.artifacts.mode);
                ok = false;
            }
            ModelFields::strings(dependency.artifacts.includeDirs,
                                 "dependency " + dependency.name + " artifact includes", diagnostics, ok);
            ModelFields::strings(dependency.artifacts.libDirs, "dependency " + dependency.name + " artifact lib_dirs",
                                 diagnostics, ok);
            ModelFields::strings(dependency.artifacts.binDirs, "dependency " + dependency.name + " artifact bin_dirs",
                                 diagnostics, ok);
            ModelFields::strings(dependency.artifacts.binFiles, "dependency " + dependency.name + " artifact bin_files",
                                 diagnostics, ok);
            ModelFields::strings(dependency.artifacts.libs, "dependency " + dependency.name + " artifact libs",
                                 diagnostics, ok);
            ModelFields::strings(dependency.artifacts.libFiles, "dependency " + dependency.name + " artifact lib_files",
                                 diagnostics, ok);
            for (const auto &[config, files] : dependency.artifacts.libFilesByConfig)
            {
                if (!ModelFields::oneOf(config, {"debug", "release", "all"}))
                {
                    diagnostics.error("dependency " + dependency.name + " artifact lib_files has invalid config " +
                                      config);
                    ok = false;
                }
                ModelFields::strings(files, "dependency " + dependency.name + " artifact lib_files." + config,
                                     diagnostics, ok);
            }

            return ok;
        }
    };

    bool ModelValidator::validate(const BuildModel &model, DiagnosticSink &diagnostics)
    {
        bool ok = true;

        if (ModelFields::blank(model.rootPackage.name))
        {
            diagnostics.error("package name is empty");
            ok = false;
        }

        std::set<std::string> dependencyNames;
        for (const auto &dependency : model.rootPackage.dependencies)
        {
            ok &= DependencyValidation::validate(dependency, dependencyNames, diagnostics);
        }

        return ok;
    }

} // namespace rhodium
