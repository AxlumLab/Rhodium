/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */
#include "rhodium/cli/Commands.hpp"

#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/Logger.hpp"
#include "rhodium/core/WorkspaceLock.hpp"

#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace rhodium
{
    int Commands::tree(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        BuildModel model;
        if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
        {
            return 1;
        }

        LockFile lockFile;
        if (!LockFile::read(workspaceRoot / "tpkg.lock.toml", lockFile, diagnostics))
        {
            return 1;
        }

        // Build dependency graph
        std::map<std::string, std::vector<std::string>> dependencyGraph;
        std::map<std::string, std::string> versions;

        for (const auto& dep : model.rootPackage.dependencies)
        {
            dependencyGraph[dep.name] = dep.dependencies;

            // Find version from lock file
            for (const auto& locked : lockFile.packages)
            {
                if (locked.name == dep.name)
                {
                    versions[dep.name] = locked.ref.empty() ? locked.commit.substr(0, 7) : locked.ref;
                    break;
                }
            }
        }

        // Print dependency tree recursively
        std::function<void(const std::string&, const std::string&, std::set<std::string>&, bool)> printTree;
        printTree = [&](const std::string& name, const std::string& prefix, std::set<std::string>& visited, bool isLast) {
            std::string connector = isLast ? "└── " : "├── ";
            std::string version = versions.count(name) ? "@" + versions[name] : "";

            LogInfo("{}{}{}{}", prefix, connector, name, version);

            if (visited.count(name))
            {
                return;  // Avoid infinite recursion
            }
            visited.insert(name);

            if (dependencyGraph.count(name))
            {
                const auto& deps = dependencyGraph[name];
                std::string newPrefix = prefix + (isLast ? "    " : "│   ");

                for (size_t i = 0; i < deps.size(); ++i)
                {
                    bool last = (i == deps.size() - 1);
                    printTree(deps[i], newPrefix, visited, last);
                }
            }
        };

        // Print root and its dependencies
        LogInfo("{}", model.rootPackage.name.empty() ? "project" : model.rootPackage.name);
        std::set<std::string> globalVisited;

        for (size_t i = 0; i < model.rootPackage.dependencies.size(); ++i)
        {
            bool isLast = (i == model.rootPackage.dependencies.size() - 1);
            std::set<std::string> branchVisited;
            printTree(model.rootPackage.dependencies[i].name, "", branchVisited, isLast);
        }

        return 0;
    }


} // namespace rhodium
