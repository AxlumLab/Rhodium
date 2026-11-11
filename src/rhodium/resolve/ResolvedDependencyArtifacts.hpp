#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium {

struct ResolvedDependencyArtifact {
    std::string name;
    std::filesystem::path root;
    std::vector<std::filesystem::path> includeDirs;
    std::vector<std::filesystem::path> libDirs;
    std::vector<std::filesystem::path> binDirs;
    std::vector<std::filesystem::path> binFiles;
    std::vector<std::string> libs;
    std::vector<std::filesystem::path> libFiles;
    std::vector<std::string> defines;
    std::vector<std::string> systemLibs;
    std::vector<std::string> frameworks;
    bool exportDefines = false;  // Export defines to consuming projects
};

struct ResolvedDependencyArtifacts {
    std::vector<ResolvedDependencyArtifact> packages;
};

} // namespace rhodium
