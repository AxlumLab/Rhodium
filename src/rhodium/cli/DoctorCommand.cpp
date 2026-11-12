/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/core/Path.hpp"

#include "rhodium/cli/Commands.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Logger.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/toolchain/ToolchainDetector.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <archive.h>
#include <curl/curl.h>
#include <git2.h>
#include <pugixml.hpp>
#include <zstd.h>

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    class DoctorCli final
    {
    public:
        static bool checkTool(const std::string &name, const std::vector<std::string> &versionArgs,
                              const std::string &reason, DiagnosticSink &diagnostics)
        {
            const auto executable = ToolchainDetector::findExecutable(name);
            if (executable.empty())
            {
                LogInfo("{}: missing", name);
                LogInfo("  required: {}", reason);
                LogInfo("  hint: install {} and make sure it is available on PATH", name);
                diagnostics.error("required core executable is missing: " + name);
                return false;
            }

            const auto result = Process::run(executable.string(), versionArgs);
            const auto version = result.exitCode == 0 ? firstLine(result.output) : std::string{};
            if (result.exitCode != 0)
            {
                LogInfo("{}: found ({}) but version check failed", name, executable.string());
                LogInfo("  required: {}", reason);
                diagnostics.error("required core executable cannot be executed: " + executable.string());
                return false;
            }

            LogInfo("{}: ok ({})", name, executable.string());
            if (!version.empty())
            {
                LogInfo("  version: {}", version);
            }
            return true;
        }

    private:
        static std::string firstLine(const std::string &text)
        {
            const auto newline = text.find_first_of("\r\n");
            auto line = newline == std::string::npos ? text : text.substr(0, newline);
            while (!line.empty() &&
                   (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            {
                line.pop_back();
            }
            return line;
        }
    };

    int Commands::doctor(const std::filesystem::path &workspaceRoot, bool sdkDetails)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        bool ok = true;

        LogInfo("Rhodium doctor");
        LogInfo("workspace: {}", workspaceRoot.string());

        const bool hasManifest = File::exists(CliWorkspace::manifest(workspaceRoot));
        LogInfo("{}: {}", CliWorkspace::manifest(workspaceRoot).filename().string(), hasManifest ? "found" : "missing");
        ok &= hasManifest;

        std::string error;
        const bool rhodiumWritable = File::mkdir(Path::stateRoot(workspaceRoot), &error);
        LogInfo("{}: {}", Path::stateRoot(workspaceRoot).filename().string(),
                rhodiumWritable ? "writable" : ("error: " + error));
        ok &= rhodiumWritable;

        LogInfo("core tools:");
        ok &= DoctorCli::checkTool("git", {"--version"}, "git is used for source acquisition and version control",
                                   diagnostics);
        ok &= DoctorCli::checkTool("cmake", {"--version"}, "cmake is required for building dependencies", diagnostics);

        LogInfo("platform: {}", Environment::hostPlatformName());
        LogInfo("compiler: {}", Environment::compilerName());
        LogInfo("build_mode: {}", Environment::buildModeName());

        auto loadedProfiles = ToolchainRegistry::load(workspaceRoot, diagnostics);
        if (loadedProfiles.empty())
        {
            std::vector<ToolchainProfile> refreshed;
            if (ToolchainDetector::writeHost(workspaceRoot, refreshed, diagnostics))
            {
                loadedProfiles = ToolchainRegistry::load(workspaceRoot, diagnostics);
            }
        }
        const auto profiles = ToolchainRegistry::profiles(loadedProfiles);
        LogInfo("toolchains: {}", profiles.empty() ? "missing" : std::to_string(profiles.size()));
        ok &= !profiles.empty();
        if (sdkDetails)
        {
            ok &= SdkDoctor::print(workspaceRoot);
        }

        if (!ok)
        {
            diagnostics.error("doctor found problems");
        }
        return ok ? 0 : 1;
    }

} // namespace rhodium
