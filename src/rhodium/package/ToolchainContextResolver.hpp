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
#include <vector>

namespace rhodium
{
    class DiagnosticSink;
}

namespace rhodium
{
    // One restore owns this snapshot; discovery and preferences are loaded once.
    class ToolchainSnapshot
    {
    public:
        explicit ToolchainSnapshot(std::filesystem::path workspaceRoot);
        ToolchainSnapshot(std::filesystem::path workspaceRoot, std::vector<ToolchainProfile> profiles);
        const std::vector<ToolchainProfile> &profiles(DiagnosticSink &diagnostics);
        bool preferredToolchain(std::string &value, DiagnosticSink &diagnostics);

    private:
        std::filesystem::path workspaceRoot_;
        std::vector<ToolchainProfile> profiles_;
        std::string preferred_;
        bool loaded_ = false;
        bool preferenceLoaded_ = false;
        bool preferenceOk_ = true;
    };

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
