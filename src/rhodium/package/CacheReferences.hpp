#pragma once

#include "rhodium/config/LockFile.hpp"

#include <filesystem>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    class CacheReferences
    {
    public:
        static bool within(const std::filesystem::path &path, const std::filesystem::path &root);
        static bool localInputs(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                const std::filesystem::path &cacheRoot, std::vector<std::filesystem::path> &inputs,
                                DiagnosticSink &diagnostics);
        static bool protectsInput(const std::filesystem::path &path, const std::vector<std::filesystem::path> &inputs);
    };
}
