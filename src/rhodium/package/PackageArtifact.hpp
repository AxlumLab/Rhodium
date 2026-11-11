/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/model/DependencyDesc.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    struct PackageArtifact
    {
        std::string name;
        std::string commit;
        std::string buildHash;
        std::string artifactId;
        std::filesystem::path root;
        DependencyArtifacts artifacts;
    };

} // namespace rhodium
