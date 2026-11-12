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
    class RHODIUM_EXPORT Sha256
    {
    public:
        static std::string file(const std::filesystem::path &path, std::string *error = nullptr);
        static bool equal(const std::string &left, const std::string &right);
    };

}
