#pragma once

#include "rhodium/model/BuildModel.hpp"
#include "rhodium/model/DependencyOverride.hpp"

#include <filesystem>
#include <string>

namespace rhodium {

class DiagnosticSink;

struct DependencyRestoreOptions
{
    bool locked = false;
    bool updateAll = false;
    bool buildOnly = false;
    bool exportOnly = false;
    bool rebuild = false;
    std::string packageName;
    std::string updatePackage;
    std::string toolchainContentHash;
};

class DependencyResolver {
public:
    bool restore(const BuildModel& model,
                 const std::filesystem::path& workspaceRoot,
                 DiagnosticSink& diagnostics,
                 const std::string& preferredToolchain = {},
                 const std::string& config = "debug",
                 const DependencyOverrideSet& overrides = {},
                 const DependencyRestoreOptions& options = {}) const;
};

} // namespace rhodium
