/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"
#include "rhodium/toolchain/Toolchain.hpp"

#include <string>

namespace rhodium
{
    class DiagnosticSink;
}

namespace rhodium
{
    class BuildToolchain
    {
    public:
        static bool populate(BuildUtil::Context &context,
                             const std::string &preferredToolchain = {},
                             const ToolchainRequirements &requirements = {},
                             DiagnosticSink *diagnostics = nullptr);

    private:
        static bool matches(const ToolchainProfile &profile, const ToolchainRequirements &requirements);
        static bool has(const ToolchainRequirements &requirements);
    };
} // namespace rhodium
