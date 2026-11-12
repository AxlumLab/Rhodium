/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/package/PatchApplier.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/toolchain/ToolchainDetector.hpp"
#include "rhodium/utils/Sha256.hpp"

#include <chrono>
#include <sstream>

namespace rhodium
{
    std::filesystem::path PatchApplier::path(const std::filesystem::path &workspaceRoot, const std::string &patch)
    {
        auto path = std::filesystem::path(patch);
        if (path.is_absolute())
        {
            return path;
        }
        return workspaceRoot / path;
    }

    std::string PatchApplier::quote(const std::filesystem::path &path)
    {
        auto text = path.string();
        std::string out = "\"";
        for (const char c : text)
        {
            if (c == '"')
            {
                out += "\\\"";
            }
            else
            {
                out.push_back(c);
            }
        }
        out += "\"";
        return out;
    }

    bool PatchApplier::git(const std::filesystem::path &sourceDir, const std::filesystem::path &patch,
                           const std::vector<std::string> &stripArgs, DiagnosticSink &diagnostics,
                           std::string &lastError)
    {
        auto git = ToolchainDetector::findExecutable("git");
        if (git.empty())
        {
            lastError = "git executable not found";
            return false;
        }

        std::vector<std::string> checkArgs = {"-C", sourceDir.string(), "apply", "--check"};
        checkArgs.insert(checkArgs.end(), stripArgs.begin(), stripArgs.end());
        checkArgs.push_back(patch.string());
        auto check = Process::run(git.string(), checkArgs);
        if (check.exitCode != 0)
        {
            lastError = check.output;
            return false;
        }

        std::vector<std::string> applyArgs = {"-C", sourceDir.string(), "apply"};
        applyArgs.insert(applyArgs.end(), stripArgs.begin(), stripArgs.end());
        applyArgs.push_back(patch.string());
        auto applied = Process::run(git.string(), applyArgs);
        if (applied.exitCode != 0)
        {
            lastError = applied.output;
            diagnostics.error("patch apply failed: " + patch.string() + "\n" + applied.output);
            return false;
        }
        return true;
    }

    bool PatchApplier::patch(const std::filesystem::path &sourceDir, const std::filesystem::path &patch,
                             DiagnosticSink &diagnostics, const std::string &lastGitError)
    {
        auto patchTool = ToolchainDetector::findExecutable("patch");
        if (patchTool.empty())
        {
            diagnostics.error("patch apply failed: neither git nor patch command is available\n" + lastGitError);
            return false;
        }
#ifdef _WIN32
        const auto command = "cd /D " + quote(sourceDir) + " && " + quote(patchTool) + " -p1 -i " + quote(patch);
        auto result = Process::runShell(command, ProcessShell::Cmd);
#else
        const auto command = "cd " + quote(sourceDir) + " && " + quote(patchTool) + " -p1 -i " + quote(patch);
        auto result = Process::runShell(command, ProcessShell::Sh);
#endif
        if (result.exitCode != 0)
        {
            diagnostics.error("patch apply failed: " + patch.string() + "\n" + lastGitError + "\n" + result.output);
            return false;
        }
        return true;
    }

    bool PatchApplier::apply(const std::filesystem::path &sourceDir, const std::filesystem::path &workspaceRoot,
                             const PatchOptions &options, std::vector<std::string> &patchHashes,
                             DiagnosticSink &diagnostics) const
    {
        const auto &[patches, stripComponents] = options;
        patchHashes.clear();
        for (const auto &patchText : patches)
        {
            const auto patchFile = path(workspaceRoot, patchText);
            if (!File::exists(patchFile))
            {
                diagnostics.error("patch not found: " + patchFile.string());
                return false;
            }
            std::string hashError;
            const auto hash = Sha256::file(patchFile, &hashError);
            if (hash.empty())
            {
                diagnostics.error("failed to hash patch " + patchFile.string() + ": " + hashError);
                return false;
            }
            patchHashes.push_back(hash);

            std::string lastGitError;
            bool applied = git(sourceDir, patchFile, {}, diagnostics, lastGitError) ||
                           git(sourceDir, patchFile, {"-p1"}, diagnostics, lastGitError) ||
                           git(sourceDir, patchFile, {"-p0"}, diagnostics, lastGitError);
            if (!applied)
            {
                applied = patch(sourceDir, patchFile, diagnostics, lastGitError);
            }
            if (!applied)
            {
                return false;
            }
        }

        std::ostringstream marker;
        marker << "strip_components=" << stripComponents << "\n";
        marker << "applied_time=" << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
        for (std::size_t i = 0; i < patches.size(); ++i)
        {
            marker << patches[i] << "=" << patchHashes[i] << "\n";
        }
        std::string error;
        if (!File::write(sourceDir / ".tpkg_patches_applied", marker.str(), &error))
        {
            diagnostics.error("failed to write patch marker: " + error);
            return false;
        }
        return true;
    }
}
