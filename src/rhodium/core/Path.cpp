/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/Path.hpp"

#include <algorithm>

namespace rhodium
{
    std::filesystem::path Path::manifest(const std::filesystem::path &workspaceRoot)
    {
        const auto preferred = workspaceRoot / "rhodium.lua";
        const auto legacy = workspaceRoot / "tpkg.lua";
        return std::filesystem::exists(preferred) || !std::filesystem::exists(legacy) ? preferred : legacy;
    }

    std::filesystem::path Path::lockFile(const std::filesystem::path &workspaceRoot)
    {
        const auto preferred = workspaceRoot / "rhodium.lock.toml";
        const auto legacy = workspaceRoot / "tpkg.lock.toml";
        return std::filesystem::exists(preferred) || !std::filesystem::exists(legacy) ? preferred : legacy;
    }

    std::filesystem::path Path::stateRoot(const std::filesystem::path &workspaceRoot)
    {
        const auto preferred = workspaceRoot / ".rhodium";
        const auto legacy = workspaceRoot / ".tpkg";
        return std::filesystem::exists(preferred) || !std::filesystem::exists(legacy) ? preferred : legacy;
    }

    std::string Path::normalizePath(const std::filesystem::path &path)
    {
        auto value = path.generic_string();
        while (value.find("//") != std::string::npos)
        {
            value.erase(value.find("//"), 1);
        }
        return value;
    }

    std::string Path::sanitizePackagePath(std::string value)
    {
        std::replace(value.begin(), value.end(), '\\', '/');
        std::replace(value.begin(), value.end(), ':', '_');
        std::replace(value.begin(), value.end(), '?', '_');
        std::replace(value.begin(), value.end(), '*', '_');
        std::replace(value.begin(), value.end(), '"', '_');
        std::replace(value.begin(), value.end(), '<', '_');
        std::replace(value.begin(), value.end(), '>', '_');
        std::replace(value.begin(), value.end(), '|', '_');
        while (!value.empty() && value.front() == '/')
        {
            value.erase(value.begin());
        }
        return value;
    }

} // namespace rhodium
