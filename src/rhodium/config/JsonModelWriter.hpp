/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/BuildModel.hpp"
#include "rhodium/resolve/ResolvedDependencyArtifacts.hpp"

#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace rhodium
{
    class RHODIUM_EXPORT JsonModel
    {
    public:
        static nlohmann::json from(const BuildModel &model);
        static nlohmann::json from(const ResolvedDependencyArtifacts &model);
        static std::string write(const BuildModel &model);
        static std::string write(const ResolvedDependencyArtifacts &model);

    private:
        static nlohmann::json dependency(const DependencyDesc &desc);
        static std::vector<std::string> paths(const std::vector<std::filesystem::path> &paths);
    };

} // namespace rhodium
