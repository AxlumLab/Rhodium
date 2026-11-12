/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{
    class RHODIUM_EXPORT File
    {
    public:
        static bool exists(const std::filesystem::path &path);
        static bool dir(const std::filesystem::path &path);
        static bool mkdir(const std::filesystem::path &path, std::string *error = nullptr);
        static bool write(const std::filesystem::path &path, const std::string &content, std::string *error = nullptr);
        static std::string read(const std::filesystem::path &path);
    };
} // namespace rhodium
