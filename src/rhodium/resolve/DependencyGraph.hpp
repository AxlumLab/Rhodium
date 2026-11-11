#pragma once

#include "rhodium/model/BuildModel.hpp"

#include <vector>

namespace rhodium
{

    class DiagnosticSink;

    class DepGraph
    {
    public:
        static bool build(const BuildModel &model,
                          std::vector<DependencyDesc> &orderedDependencies,
                          DiagnosticSink &diagnostics);
    };

} // namespace rhodium
