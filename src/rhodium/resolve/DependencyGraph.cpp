#include "rhodium/resolve/DependencyGraph.hpp"

#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <functional>
#include <map>
#include <set>

namespace rhodium
{

    bool DepGraph::build(const BuildModel &model,
                         std::vector<DependencyDesc> &orderedDependencies,
                         DiagnosticSink &diagnostics)
    {
        orderedDependencies.clear();

        std::set<std::string> visited;
        std::set<std::string> inProgress;
        std::map<std::string, DependencyDesc> dependencyMap;

        for (const auto &dep : model.rootPackage.dependencies)
        {
            dependencyMap[dep.name] = dep;
        }

        std::function<bool(const std::string &)> collectDependencies;
        collectDependencies = [&](const std::string &name) -> bool {
            if (visited.count(name))
            {
                return true;
            }

            if (inProgress.count(name))
            {
                diagnostics.error("circular dependency detected: " + name);
                return false;
            }

            auto it = dependencyMap.find(name);
            if (it == dependencyMap.end())
            {
                diagnostics.error("dependency not found: " + name);
                return false;
            }

            const auto &dep = it->second;
            inProgress.insert(name);

            for (const auto &transitiveName : dep.dependencies)
            {
                if (!collectDependencies(transitiveName))
                {
                    return false;
                }
            }

            inProgress.erase(name);
            visited.insert(name);
            orderedDependencies.push_back(dep);
            return true;
        };

        for (const auto &dependency : model.rootPackage.dependencies)
        {
            if (!collectDependencies(dependency.name))
            {
                return false;
            }
        }

        return true;
    }

} // namespace rhodium
