/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include <filesystem>
#include <string>

namespace rhodium
{
    std::string sha256File(const std::filesystem::path &path, std::string *error = nullptr);
    bool sha256Equals(const std::string &left, const std::string &right);
}
