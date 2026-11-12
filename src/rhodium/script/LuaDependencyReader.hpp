/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/BuildModel.hpp"

#include <sol/sol.hpp>

#include <filesystem>
#include <string>

namespace rhodium
{

    class DiagnosticSink;

    class RHODIUM_EXPORT LuaDependencyReader
    {
    public:
        LuaDependencyReader(const std::string &activePlatform, const std::filesystem::path &workspaceRoot)
            : activePlatform(activePlatform), workspaceRoot(workspaceRoot)
        {
        }

        DependencyDesc read(const std::string &idOrSource, sol::table options, bool local) const;
        static void overlay(sol::table options, DependencyDesc &dependency);
        static void overrides(sol::table table, BuildModel &model, DiagnosticSink &diagnostics);

    private:
        std::string activePlatform;
        std::filesystem::path workspaceRoot;
    };

} // namespace rhodium
