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

    struct LuaBindingOptions
    {
        std::string activePlatform;
        std::filesystem::path workspaceRoot;
        std::filesystem::path manifestPath;
    };

    class LuaBindings
    {
    public:
        static void bind(sol::state &lua, BuildModel &model, DiagnosticSink &diagnostics,
                         const LuaBindingOptions &options);
    };

} // namespace rhodium
