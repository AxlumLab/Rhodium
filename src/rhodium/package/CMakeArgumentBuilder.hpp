/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace rhodium
{
    class RHODIUM_EXPORT CMakeArgs
    {
    public:
        static std::map<std::string, std::string> options(const DependencyDesc &dependency);
        static std::vector<std::string> configure(const DependencyDesc &dependency, const BuildUtil::Context &context);
        static std::vector<std::string> wrap(const std::filesystem::path &workingDir,
                                             const std::map<std::string, std::string> &env,
                                             const std::string &executable,
                                             const std::vector<std::string> &args);

    private:
        static std::string runtime(const std::string &value);
        static std::string boolOpt(const std::string &value);
        static void setDefault(std::map<std::string, std::string> &options,
                               const std::string &key,
                               const std::string &value);
    };
} // namespace rhodium
