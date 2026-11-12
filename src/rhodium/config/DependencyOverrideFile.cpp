/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/core/Path.hpp"

#include "rhodium/config/DependencyOverrideFile.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <exception>
#include <sstream>
#include <toml++/toml.hpp>

namespace rhodium
{
    class OverrideData final
    {
    public:
        OverrideData(DependencyOverrideSet &set, DiagnosticSink &diagnostics) : set(set), diagnostics(diagnostics) {}

        bool add(const std::string &name, const std::string &rawPath, const std::filesystem::path &base,
                 const std::string &source)
        {
            if (name.empty())
            {
                diagnostics.error("dependency override name is empty");
                return false;
            }
            if (rawPath.empty())
            {
                diagnostics.error("dependency override path is empty for " + name);
                return false;
            }
            auto path = std::filesystem::path(rawPath);
            if (path.is_relative())
            {
                path = base / path;
            }
            path = normalize(path);
            if (!File::dir(path))
            {
                diagnostics.error("dependency override path is not a directory for " + name + ": " + path.string());
                return false;
            }

            DependencyOverride override;
            override.name = name;
            override.rawPath = rawPath;
            override.path = path;
            override.source = source;
            set.overrides[name] = std::move(override);
            return true;
        }

        static bool parseCli(const std::string &value, std::string &name, std::string &path,
                             DiagnosticSink &diagnostics)
        {
            const auto equals = value.find('=');
            if (equals == std::string::npos || equals == 0 || equals + 1 >= value.size())
            {
                diagnostics.error("dependency override must be name=path: " + value);
                return false;
            }
            name = value.substr(0, equals);
            path = value.substr(equals + 1);
            return true;
        }

    private:
        DependencyOverrideSet &set;
        DiagnosticSink &diagnostics;

        static std::filesystem::path normalize(const std::filesystem::path &path)
        {
            std::error_code ec;
            auto absolute = std::filesystem::absolute(path, ec);
            if (ec)
            {
                absolute = path;
            }
            auto canonical = std::filesystem::weakly_canonical(absolute, ec);
            return (ec ? absolute : canonical).lexically_normal();
        }
    };

    std::filesystem::path DependencyOverrides::path(const std::filesystem::path &workspaceRoot)
    {
        return Path::stateRoot(workspaceRoot) / "overrides.toml";
    }

    bool DependencyOverrides::read(const std::filesystem::path &workspaceRoot, LocalDependencyOverrides &overrides,
                                   DiagnosticSink &diagnostics)
    {
        overrides.paths.clear();
        const auto file = path(workspaceRoot);
        if (!File::exists(file))
        {
            return true;
        }
        try
        {
            auto table = toml::parse_file(file.string());
            const auto *overrideTable = table["overrides"].as_table();
            if (!overrideTable)
            {
                return true;
            }
            for (const auto &[key, value] : *overrideTable)
            {
                if (auto stringValue = value.value<std::string>())
                {
                    overrides.paths[std::string(key.str())] = *stringValue;
                }
                else if (const auto *object = value.as_table())
                {
                    overrides.paths[std::string(key.str())] = (*object)["path"].value_or("");
                }
            }
            return true;
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to read dependency overrides: " + std::string(ex.what()));
            return false;
        }
    }

    bool DependencyOverrides::write(const std::filesystem::path &workspaceRoot,
                                    const LocalDependencyOverrides &overrides, DiagnosticSink &diagnostics)
    {
        std::ostringstream stream;
        stream << "[overrides]\n";
        for (const auto &[name, path] : overrides.paths)
        {
            stream << toml::value<std::string>(name) << " = " << toml::value<std::string>(path) << "\n";
        }

        std::string error;
        if (!File::mkdir(path(workspaceRoot).parent_path(), &error))
        {
            diagnostics.error("failed to create state directory: " + error);
            return false;
        }
        if (!File::write(path(workspaceRoot), stream.str(), &error))
        {
            diagnostics.error("failed to write dependency overrides: " + error);
            return false;
        }
        return true;
    }

    bool DependencyOverrides::make(const BuildModel &model, const std::filesystem::path &workspaceRoot,
                                   const std::vector<std::string> &cliOverrides, DependencyOverrideSet &overrides,
                                   DiagnosticSink &diagnostics)
    {
        overrides.overrides.clear();
        OverrideData entries(overrides, diagnostics);
        for (const auto &override : model.rootPackage.dependencyOverrides)
        {
            if (!entries.add(override.name, override.path, workspaceRoot, "dsl"))
            {
                return false;
            }
        }

        LocalDependencyOverrides local;
        if (!read(workspaceRoot, local, diagnostics))
        {
            return false;
        }
        for (const auto &[name, path] : local.paths)
        {
            if (!entries.add(name, path, workspaceRoot, "local"))
            {
                return false;
            }
        }

        for (const auto &value : cliOverrides)
        {
            std::string name;
            std::string path;
            if (!OverrideData::parseCli(value, name, path, diagnostics))
            {
                return false;
            }
            if (!entries.add(name, path, std::filesystem::current_path(), "cli"))
            {
                return false;
            }
        }
        return true;
    }

} // namespace rhodium
