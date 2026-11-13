/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/config/TomlConfig.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Path.hpp"

#include <toml++/toml.hpp>

namespace rhodium
{

    std::optional<ToolConfig> ToolConfig::load(const std::filesystem::path &path)
    {
        if (!File::exists(path))
        {
            return std::nullopt;
        }

        auto table = toml::parse_file(path.string());
        ToolConfig config;
        config.buildDir = (Path::stateRoot(std::filesystem::path{}) / "build").generic_string();
        config.buildDir = table["build_dir"].value_or(config.buildDir);
        config.defaultPlatform = table["default_platform"].value_or("");
        config.defaultConfig = table["default_config"].value_or("");
        config.defaultArch = table["default_arch"].value_or("");
        return config;
    }

} // namespace rhodium
