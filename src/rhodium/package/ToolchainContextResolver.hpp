/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"
#include "rhodium/toolchain/Toolchain.hpp"
#include <vector>

#include <string>

namespace rhodium
{
    class DiagnosticSink;
}

namespace rhodium
{
    class RHODIUM_EXPORT BuildToolchain
    {
    public:
        static bool populate(BuildUtil::Context &context, const std::string &preferredToolchain = {},
                             const ToolchainRequirements &requirements = {}, DiagnosticSink *diagnostics = nullptr);

    private:
        static const ToolchainProfile *select(const std::vector<ToolchainProfile> &profiles,
                                              const std::string &requested, const ToolchainRequirements &requirements,
                                              DiagnosticSink &sink);
        static void apply(BuildUtil::Context &context, const ToolchainProfile &selected);
        static bool matches(const ToolchainProfile &profile, const ToolchainRequirements &requirements);
        static bool has(const ToolchainRequirements &requirements);
    };
} // namespace rhodium
