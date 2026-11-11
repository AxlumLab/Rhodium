#pragma once

#include <filesystem>
#include <string>

namespace rhodium
{
    class Path
    {
    public:
        // Prefer Rhodium names; reuse legacy names only when the new name is absent.
        static std::filesystem::path manifest(const std::filesystem::path &workspaceRoot);
        static std::filesystem::path stateRoot(const std::filesystem::path &workspaceRoot);
        static std::string normalizePath(const std::filesystem::path &path);
        static std::string sanitizePackagePath(std::string value);
    };
} // namespace rhodium
