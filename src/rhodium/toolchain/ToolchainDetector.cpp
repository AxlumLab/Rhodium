/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/Path.hpp"

#include "rhodium/toolchain/ToolchainDetector.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/toolchain/ToolchainProbe.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <filesystem>
#include <vector>

namespace rhodium
{
    std::vector<ToolchainProfile> ToolchainDetector::host(const std::filesystem::path &workspaceRoot,
                                                          DiagnosticSink &diagnostics)
    {
        std::vector<ToolchainProfile> profiles;
        const auto platform = Environment::hostPlatformName();
        if (platform == "windows")
        {
            ToolchainDetector::windows(profiles, diagnostics);
        }
        else
        {
            ToolchainDetector::unixProfiles(profiles);
        }
        ToolchainDetector::android(profiles, workspaceRoot, diagnostics);

        if (profiles.empty())
        {
            diagnostics.warning("no C/C++ toolchains detected");
        }
        return profiles;
    }

    std::filesystem::path ToolchainDetector::cachePath(const std::filesystem::path &workspaceRoot)
    {
        return Path::stateRoot(workspaceRoot) / "toolchains" / "host.toml";
    }

    bool ToolchainDetector::writeHost(const std::filesystem::path &workspaceRoot,
                                      std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        profiles = host(workspaceRoot, diagnostics);
        return ToolchainRegistry::write(cachePath(workspaceRoot), profiles, diagnostics);
    }

    std::vector<ToolchainProfile> ToolchainDetector::host(DiagnosticSink &diagnostics)
    {
        return ToolchainDetector::host({}, diagnostics);
    }

}
