/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/export.hpp"

#include <map>
#include <string>
#include <vector>

namespace rhodium
{
    enum class ProcessShell
    {
        Direct,
        Cmd,
        PowerShell,
        Sh,
    };

    struct ProcessResult
    {
        int exitCode = -1;
        std::string output;
    };

    class RHODIUM_EXPORT Process
    {
    public:
        static ProcessResult run(const std::string &executable, const std::vector<std::string> &args);
        static ProcessResult run(const std::string &executable, const std::vector<std::string> &args, const std::map<std::string, std::string> &environment);
        static ProcessResult runShell(const std::string &command, ProcessShell shell);
        static ProcessResult runShell(const std::string &command, ProcessShell shell, const std::map<std::string, std::string> &environment);
    };

} // namespace rhodium
