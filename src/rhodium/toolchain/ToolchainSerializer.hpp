/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace rhodium
{
    // Utility class for toolchain configuration serialization
    class RHODIUM_EXPORT ToolchainSerializer
    {
    public:
        // Append path to TOML stream
        static void appendPath(std::ostringstream &stream,
                              const char *name,
                              const std::filesystem::path &path);

        // Append string to TOML stream
        static void appendString(std::ostringstream &stream,
                                const char *name,
                                const std::string &value);

        // Append string array to TOML stream
        static void appendStringArray(std::ostringstream &stream,
                                     const char *name,
                                     const std::vector<std::string> &values);

        // Append path array to TOML stream
        static void appendPathArray(std::ostringstream &stream,
                                   const char *name,
                                   const std::vector<std::filesystem::path> &paths);

    private:
        ToolchainSerializer() = delete; // Utility class, no instances
    };

} // namespace rhodium
