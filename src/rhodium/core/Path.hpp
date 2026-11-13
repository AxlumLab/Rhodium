/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{
    class RHODIUM_EXPORT Path
    {
    public:
        // Prefer Rhodium names; reuse legacy names only when the new name is absent.
        static std::filesystem::path manifest(const std::filesystem::path &workspaceRoot);
        static std::filesystem::path lockFile(const std::filesystem::path &workspaceRoot);
        static std::filesystem::path stateRoot(const std::filesystem::path &workspaceRoot);
        static std::string normalizePath(const std::filesystem::path &path);
        static std::string sanitizePackagePath(std::string value);
    };
} // namespace rhodium
