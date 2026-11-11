/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/model/BuildModel.hpp"

#include <filesystem>
#include <string>

namespace sol
{
    class state;
}

namespace rhodium
{

    class DiagnosticSink;

    void bindDsl(sol::state &lua,
                 BuildModel &model,
                 DiagnosticSink &diagnostics,
                 const std::string &activePlatform,
                 const std::filesystem::path &workspaceRoot = {},
                 const std::filesystem::path &manifestPath = {});

} // namespace rhodium
