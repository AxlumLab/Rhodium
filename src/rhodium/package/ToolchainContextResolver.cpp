/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/ToolchainContextResolver.hpp"

#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/toolchain/ToolchainDetector.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <filesystem>
#include <map>
#include <vector>

namespace rhodium
{
    namespace
    {
        class BuildProfiles final
        {
        public:
            static std::vector<ToolchainProfile> load(const std::filesystem::path &workspaceRoot,
                                                      DiagnosticSink &diagnostics)
            {
                std::vector<ToolchainProfile> profiles = ToolchainDetector::host(workspaceRoot, diagnostics);
                if (workspaceRoot.empty())
                {
                    return profiles;
                }

                std::map<std::string, std::size_t> byId;
                for (std::size_t index = 0; index < profiles.size(); ++index)
                {
                    if (!profiles[index].id.empty())
                    {
                        byId[profiles[index].id] = index;
                    }
                }

                for (const auto &loaded : ToolchainRegistry::load(workspaceRoot, diagnostics))
                {
                    if (loaded.profile.id.empty())
                    {
                        continue;
                    }
                    const auto found = byId.find(loaded.profile.id);
                    if (loaded.source == ToolchainSource::AutoDetected && found != byId.end())
                    {
                        continue;
                    }
                    if (found == byId.end())
                    {
                        byId[loaded.profile.id] = profiles.size();
                        profiles.push_back(loaded.profile);
                    }
                    else
                    {
                        profiles[found->second] = loaded.profile;
                    }
                }
                return profiles;
            }
        };
    }

    bool BuildToolchain::matches(const ToolchainProfile &profile, const ToolchainRequirements &requirements)
    {
        const auto family = StringUtils::toLower(requirements.family);
        const auto abi = StringUtils::toLower(requirements.abi);
        const auto binaryFormat = StringUtils::toLower(requirements.binaryFormat);
        const auto compilerKind = StringUtils::toLower(profile.compilerKind);
        const auto sdkKind = StringUtils::toLower(profile.sdkKind);
        const auto id = StringUtils::toLower(profile.id);

        if (!family.empty())
        {
            if (family == "msvc-abi" || family == "msvc")
            {
                if (compilerKind != "msvc" && compilerKind != "clang-cl")
                {
                    return false;
                }
            }
            else if (family == "gnu" || family == "mingw")
            {
                if (compilerKind != "gcc" && compilerKind != "clang")
                {
                    return false;
                }
                if (family == "mingw" && sdkKind != "mingw" && id.find("mingw") == std::string::npos)
                {
                    return false;
                }
            }
            else if (family != compilerKind)
            {
                return false;
            }
        }
        if (!abi.empty())
        {
            if (abi == "msvc")
            {
                if (compilerKind != "msvc" && compilerKind != "clang-cl")
                {
                    return false;
                }
            }
            else if (abi == "gnu" || abi == "mingw")
            {
                if (compilerKind != "gcc" && compilerKind != "clang")
                {
                    return false;
                }
            }
            else if (abi != compilerKind)
            {
                return false;
            }
        }
        if (!binaryFormat.empty())
        {
            if (binaryFormat == "coff")
            {
                if (compilerKind != "msvc" && compilerKind != "clang-cl")
                {
                    return false;
                }
            }
            else if (binaryFormat == "archive" || binaryFormat == "ar")
            {
                if (compilerKind == "msvc" || compilerKind == "clang-cl")
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool BuildToolchain::has(const ToolchainRequirements &requirements)
    {
        return !requirements.family.empty() || !requirements.abi.empty() || !requirements.binaryFormat.empty();
    }

    const ToolchainProfile *BuildToolchain::select(const std::vector<ToolchainProfile> &profiles,
                                                   const std::string &requested,
                                                   const ToolchainRequirements &requirements, DiagnosticSink &sink)
    {
        const ToolchainProfile *selected = nullptr;
        if (!requested.empty())
        {
            for (const auto &profile : profiles)
            {
                if (profile.id == requested)
                {
                    selected = &profile;
                    break;
                }
            }
            if (!selected)
            {
                sink.error("requested toolchain was not detected: " + requested);
                return nullptr;
            }
            if (!matches(*selected, requirements))
            {
                sink.error("selected toolchain does not satisfy package toolchain requirements: " + selected->id);
                return nullptr;
            }
        }
        else
        {
            for (const auto &profile : profiles)
            {
                if (profile.complete && matches(profile, requirements))
                {
                    selected = &profile;
                    break;
                }
            }
            if (!selected && !has(requirements))
            {
                for (const auto &profile : profiles)
                {
                    if (matches(profile, requirements))
                    {
                        selected = &profile;
                        break;
                    }
                }
            }
            if (!selected)
            {
                sink.error("no complete detected toolchain satisfies package toolchain requirements; pass --toolchain "
                           "<id> to select a local profile explicitly");
                return nullptr;
            }
        }

        return selected;
    }

    void BuildToolchain::apply(BuildUtil::Context &context, const ToolchainProfile &selected)
    {
        context.platform = selected.platform;
        context.arch = selected.targetArch;
        context.cc = selected.compiler.string();
        context.cxx = selected.cxxCompiler.empty() ? selected.compiler.string() : selected.cxxCompiler.string();
        context.linker = selected.linker.string();
        context.archiver = selected.archiver.string();
        context.rc = selected.resourceCompiler.string();
        context.mt = selected.manifestTool.string();
        context.cmake = selected.cmake.empty() ? "cmake" : selected.cmake.string();
        context.ninja = selected.ninja.empty() ? "ninja" : selected.ninja.string();
        context.git = selected.git.empty() ? "git" : selected.git.string();
        context.environment = selected.environment;
    }

    bool BuildToolchain::populate(BuildUtil::Context &context, const std::string &preferredToolchain,
                                  const ToolchainRequirements &requirements, DiagnosticSink *providedDiagnostics)
    {
        class NullDiagnosticSink final : public DiagnosticSink
        {
        public:
            void report(const Diagnostic &) override {}
        };

        NullDiagnosticSink diagnostics;
        DiagnosticSink &sink = providedDiagnostics ? *providedDiagnostics : diagnostics;
        const auto profiles = BuildProfiles::load(context.workspaceRoot, sink);
        if (profiles.empty())
        {
            return false;
        }

        std::string requested = preferredToolchain;
        if (requested.empty() && !context.workspaceRoot.empty())
        {
            LocalToolchainConfig config;
            if (!ToolchainRegistry::readConfig(context.workspaceRoot, config, sink))
            {
                return false;
            }
            requested = config.preferredToolchain;
        }

        const auto *selected = select(profiles, requested, requirements, sink);
        if (!selected)
        {
            return false;
        }
        sink.info("selected toolchain: " + selected->id);
        apply(context, *selected);
        return true;
    }

} // namespace rhodium
