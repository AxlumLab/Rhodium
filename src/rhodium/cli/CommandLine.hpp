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
    class CmdLine
    {
    public:
        static int run(int argc, char **argv);
        static std::string version();

    private:
        static std::filesystem::path root(const std::string &value);
    };

} // namespace rhodium
