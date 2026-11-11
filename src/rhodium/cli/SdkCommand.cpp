#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/cli/Commands.hpp"
#include "rhodium/cli/CommandContext.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Logger.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/core/StringUtil.hpp"
#include "rhodium/toolchain/ToolchainDetector.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <fmt/color.h>
#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace rhodium
{
    class SdkCli final
    {
    public:
        static void print(const ToolchainProfile &profile)
        {
            LogInfo("[{}]", profile.id);
            LogInfo("  compiler_kind: {}", profile.compilerKind);
            LogInfo("  compiler: {}", profile.compiler.empty() ? "-" : profile.compiler.string());
            LogInfo("  cxx_compiler: {}", profile.cxxCompiler.empty() ? "-" : profile.cxxCompiler.string());
            LogInfo("  linker: {}", profile.linker.empty() ? "-" : profile.linker.string());
            LogInfo("  archiver: {}", profile.archiver.empty() ? "-" : profile.archiver.string());
            LogInfo("  sdk: {} {}", profile.sdkKind, profile.sdkVersion.empty() ? "-" : profile.sdkVersion);
            LogInfo("  sdk_root: {}", profile.sdkRoot.empty() ? "-" : profile.sdkRoot.string());
            LogInfo("  sysroot: {}", profile.sysroot.empty() ? "-" : profile.sysroot.string());
            if (!profile.targetTriple.empty())
            {
                LogInfo("  target_triple: {}", profile.targetTriple);
            }
            if (!profile.deploymentTarget.empty())
            {
                LogInfo("  deployment_target: {}", profile.deploymentTarget);
            }
            if (!profile.androidAbi.empty())
            {
                LogInfo("  android_abi: {}", profile.androidAbi);
            }
            if (!profile.androidApi.empty())
            {
                LogInfo("  android_api: {}", profile.androidApi);
            }
            if (!profile.androidSdkRoot.empty())
            {
                LogInfo("  android_sdk_root: {}", profile.androidSdkRoot.string());
            }
            if (!profile.androidNdkRoot.empty())
            {
                LogInfo("  android_ndk_root: {}", profile.androidNdkRoot.string());
            }
            LogInfo("  status: {}", profile.complete ? "complete" : "incomplete");
            if (!profile.missing.empty())
            {
                LogInfo("  missing: {}", join(profile.missing, ", "));
            }
            else
            {
                LogInfo("  missing: -");
            }
        }

        static std::filesystem::path androidSysroot(const ToolchainProfile &profile)
        {
            if (!profile.sysroot.empty())
            {
                return profile.sysroot;
            }
            const auto compiler = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
            if (compiler.empty())
            {
                return {};
            }
            const auto sysroot = compiler.parent_path().parent_path() / "sysroot";
            return File::dir(sysroot) ? sysroot : std::filesystem::path{};
        }

        static bool hasLibDir(const ToolchainProfile &profile, const std::string &needle)
        {
            return std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(), [&](const auto &dir)
                               { return pathHas(dir, needle); });
        }

        static std::string cxx(const ToolchainProfile &profile)
        {
            const auto compiler = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
            return compiler.string();
        }

        static std::vector<std::string> msvcCompileArgs(const ToolchainProfile &profile,
                                                        const std::filesystem::path &source,
                                                        const std::filesystem::path &object,
                                                        const std::string &config)
        {
            std::vector<std::string> args = {
                "/nologo",
                "/std:c++20",
                "/EHsc",
                StringUtils::toLower(config) == "debug" ? "/MDd" : "/MD",
                "/c",
                source.string(),
                "/Fo" + object.string(),
            };
            for (const auto &includeDir : profile.systemIncludeDirs)
            {
                args.push_back("/I" + includeDir.string());
            }
            return args;
        }

        static std::vector<std::string> msvcLinkArgs(const ToolchainProfile &profile,
                                                     const std::filesystem::path &exe,
                                                     const std::filesystem::path &object)
        {
            std::vector<std::string> args = {
                "/NOLOGO",
                "/OUT:" + exe.string(),
                object.string(),
            };
            for (const auto &libDir : profile.systemLibDirs)
            {
                args.push_back("/LIBPATH:" + libDir.string());
            }
            return args;
        }

        static std::vector<std::filesystem::path> androidCandidates()
        {
            std::vector<std::filesystem::path> candidates;
            auto add = [&](const std::filesystem::path &path)
            {
                if (!path.empty() && std::find(candidates.begin(), candidates.end(), path) == candidates.end())
                {
                    candidates.push_back(path);
                }
            };
            add(env("ANDROID_HOME"));
            add(env("ANDROID_SDK_ROOT"));
#ifdef _WIN32
            const auto localAppData = env("LOCALAPPDATA");
            if (!localAppData.empty())
            {
                add(std::filesystem::path(localAppData) / "Android" / "Sdk");
            }
            const auto userProfile = env("USERPROFILE");
            if (!userProfile.empty())
            {
                add(std::filesystem::path(userProfile) / "AppData" / "Local" / "Android" / "Sdk");
            }
#else
            const auto home = env("HOME");
            if (!home.empty())
            {
                add(std::filesystem::path(home) / "Library" / "Android" / "sdk");
                add(std::filesystem::path(home) / "Android" / "Sdk");
                add(std::filesystem::path(home) / "Android" / "sdk");
            }
            add("/opt/android-sdk");
#endif
            return candidates;
        }

        static bool hasAndroidSignal()
        {
            if (!env("ANDROID_HOME").empty() || !env("ANDROID_SDK_ROOT").empty())
            {
                return true;
            }
            for (const auto &candidate : androidCandidates())
            {
                if (File::dir(candidate))
                {
                    return true;
                }
            }
            return false;
        }

        static bool hasAndroidProfile(const std::vector<ToolchainProfile> &profiles)
        {
            return std::any_of(profiles.begin(), profiles.end(), [](const auto &profile)
                               { return profile.platform == "android" || profile.id.rfind("android-", 0) == 0; });
        }

        static bool msvcProbe(const ToolchainProfile &profile, std::string &output)
        {
            if (profile.compilerKind != "msvc" && profile.compilerKind != "clang-cl")
            {
                return true;
            }
            if (cxx(profile).empty() || profile.linker.empty())
            {
                output = "compiler or linker missing";
                return false;
            }

            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto dir = std::filesystem::temp_directory_path() / ("rhodium-sdk-probe-" + std::to_string(stamp));
            const auto source = dir / "test.cpp";
            std::string error;
            if (!File::write(source, "#include <iostream>\nint main(){std::cout << \"ok\\n\"; return 0;}\n", &error))
            {
                output = error;
                return false;
            }

            bool ok = true;
            for (const auto &config : {std::string("debug"), std::string("release")})
            {
                const auto object = dir / ("test-" + config + ".obj");
                const auto exe = dir / ("test-" + config + ".exe");
                auto compile = Process::run(cxx(profile),
                                            msvcCompileArgs(profile, source, object, config),
                                            profile.environment);
                output += "[" + config + " compile]\n" + compile.output;
                if (compile.exitCode != 0 || !File::exists(object))
                {
                    ok = false;
                    break;
                }
                auto link = Process::run(profile.linker.string(),
                                         msvcLinkArgs(profile, exe, object),
                                         profile.environment);
                output += "[" + config + " link]\n" + link.output;
                if (link.exitCode != 0 || !File::exists(exe))
                {
                    ok = false;
                    break;
                }
            }
            try
            {
                std::filesystem::remove_all(dir);
            }
            catch (...)
            {
            }
            return ok;
        }

        static bool androidProbe(const ToolchainProfile &profile, std::string &output)
        {
            if (profile.platform != "android" && profile.sdkKind != "android-ndk")
            {
                return true;
            }
            const auto compiler = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
            if (compiler.empty())
            {
                output = "compiler missing";
                return false;
            }

            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto dir = std::filesystem::temp_directory_path() / ("rhodium-android-sdk-probe-" + std::to_string(stamp));
            const auto source = dir / "test.cpp";
            const auto object = dir / "test.o";
            std::string error;
            if (!File::write(source, "int add(int a, int b) { return a + b; }\n", &error))
            {
                output = error;
                return false;
            }

            std::vector<std::string> args = {"-std=c++20", "-DANDROID", "-fPIC", "-c", source.string(), "-o", object.string()};
            const auto result = Process::run(compiler.string(), args, profile.environment);
            output = result.output;
            const bool ok = result.exitCode == 0 && File::exists(object);
            try
            {
                std::filesystem::remove_all(dir);
            }
            catch (...)
            {
            }
            return ok;
        }

        static void printPaths(const std::string &label, const std::vector<std::filesystem::path> &paths)
        {
            LogInfo("{}:", label);
            if (paths.empty())
            {
                LogInfo("  -");
                return;
            }
            for (const auto &path : paths)
            {
                LogInfo("  {}", path.string());
            }
        }

        static bool applyEnv(ToolchainProfile &profile, const std::vector<std::string> &values, DiagnosticSink &diagnostics)
        {
            for (const auto &value : values)
            {
                const auto equals = value.find('=');
                if (equals == std::string::npos || equals == 0)
                {
                    diagnostics.error("sdk add --env must be KEY=VALUE: " + value);
                    return false;
                }
                profile.environment[value.substr(0, equals)] = value.substr(equals + 1);
            }
            return true;
        }

        static std::string join(const std::vector<std::string> &values, const std::string &separator)
        {
            std::ostringstream stream;
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                if (i != 0)
                {
                    stream << separator;
                }
                stream << values[i];
            }
            return stream.str();
        }

    private:
        static bool pathHas(const std::filesystem::path &path, const std::string &needle)
        {
            auto text = path.generic_string();
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });
            return text.find(needle) != std::string::npos;
        }

        static std::string env(const char *key)
        {
#ifdef _WIN32
            char *value = nullptr;
            std::size_t size = 0;
            if (_dupenv_s(&value, &size, key) != 0 || !value)
            {
                return {};
            }
            std::string result(value);
            std::free(value);
            return result;
#else
            const char *value = std::getenv(key);
            return value ? std::string(value) : std::string{};
#endif
        }
    };

    bool printSdkDoctor(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
        if (loadedProfiles.empty())
        {
            std::vector<ToolchainProfile> refreshed;
            if (!detectAndWriteHostToolchains(workspaceRoot, refreshed, diagnostics))
            {
                return false;
            }
            loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
        }
        const auto profiles = unwrapToolchainProfiles(loadedProfiles);

        LogInfo("toolchain_cache: {}", hostToolchainCachePath(workspaceRoot).string());
        if (profiles.empty())
        {
            LogInfo("toolchains: none");
            return false;
        }

        bool ok = true;
        const bool anyAndroidProfile = SdkCli::hasAndroidProfile(profiles);
        for (const auto &loaded : loadedProfiles)
        {
            const auto &profile = loaded.profile;
            SdkCli::print(profile);
            LogInfo("  source: {}", toString(loaded.source));
            LogInfo("  source_file: {}", loaded.sourceFile.string());
            if (!loaded.overridden.empty())
            {
                LogInfo("  overrides:");
                for (const auto &overridden : loaded.overridden)
                {
                    LogInfo("    {}: {}", toString(overridden.source), overridden.sourceFile.string());
                }
            }

            LogInfo("  include_dirs: {}", profile.systemIncludeDirs.size());
            LogInfo("  lib_dirs: {}", profile.systemLibDirs.size());
            LogInfo("  bin_dirs: {}", profile.binaryDirs.size());

            LogInfo("  git: {}", fmt::styled(
                                     profile.git.empty() ? std::string("missing") : profile.git.string(),
                                     fmt::fg(profile.git.empty() ? fmt::color::red : fmt::color::green)));

            LogInfo("  ninja: {}", fmt::styled(
                                       profile.ninja.empty() ? std::string("missing") : profile.ninja.string(),
                                       fmt::fg(profile.ninja.empty() ? fmt::color::red : fmt::color::green)));

            if (profile.git.empty())
            {
                ok = false;
                LogInfo("  git_required: dependency restore and source acquisition may fail");
            }
            if (profile.ninja.empty())
            {
                ok = false;
                LogInfo("  ninja_required: package builds may fail");
            }
            if (profile.platform == "android")
            {
                LogInfo("  android_sdk_root: {}", profile.androidSdkRoot.empty() ? "-" : profile.androidSdkRoot.string());
                LogInfo("  android_ndk_root: {}", profile.androidNdkRoot.empty() ? "-" : profile.androidNdkRoot.string());
                LogInfo("  jdk_root: {}", profile.jdkRoot.empty() ? "-" : profile.jdkRoot.string());
                LogInfo("  android_api: {}", profile.androidApi.empty() ? "-" : profile.androidApi);
                LogInfo("  android_abi: {}", profile.androidAbi.empty() ? "-" : profile.androidAbi);
                LogInfo("  target_triple: {}", profile.targetTriple.empty() ? "-" : profile.targetTriple);
                const auto hostPrebuilt = profile.androidNdkRoot.empty() ? std::filesystem::path{} : profile.androidNdkRoot / "toolchains" / "llvm" / "prebuilt";
                LogInfo("  host_prebuilt: {}", hostPrebuilt.empty() ? "-" : hostPrebuilt.string());
                const auto sysroot = SdkCli::androidSysroot(profile);
                LogInfo("  sysroot: {}", sysroot.empty() ? "-" : sysroot.string());
                const auto compilerName = profile.compiler.filename().string();
                const auto compilerMode = !profile.targetTriple.empty() && compilerName.find(profile.targetTriple) != std::string::npos
                                              ? "wrapper"
                                              : "clang-with-target";
                LogInfo("  compiler_mode: {}", profile.compiler.empty() ? "-" : compilerMode);
                LogInfo("  compiler: {}", profile.compiler.empty() ? "-" : profile.compiler.string());
                LogInfo("  cxx_compiler: {}", profile.cxxCompiler.empty() ? "-" : profile.cxxCompiler.string());
                LogInfo("  archiver: {}", profile.archiver.empty() ? "-" : profile.archiver.string());
                LogInfo("  adb: {}", profile.adb.empty() ? "-" : profile.adb.string());
                LogInfo("  java: {}", profile.java.empty() ? "-" : profile.java.string());
                LogInfo("  javac: {}", profile.javac.empty() ? "-" : profile.javac.string());
                LogInfo("  gradle: {}", profile.gradle.empty() ? "-" : profile.gradle.string());
            }
            if (profile.platform == "macos" || profile.platform == "ios")
            {
                LogInfo("  libtool: {}", profile.libtool.empty() ? "-" : profile.libtool.string());
                LogInfo("  lipo: {}", profile.lipo.empty() ? "-" : profile.lipo.string());
                LogInfo("  codesign: {}", profile.codesign.empty() ? "-" : profile.codesign.string());
            }
            if (profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl")
            {
                LogInfo("  has vc lib: {}", SdkCli::hasLibDir(profile, "/vc/tools/msvc/") ? "yes" : "no");
                LogInfo("  has ucrt lib: {}", SdkCli::hasLibDir(profile, "/ucrt/") ? "yes" : "no");
                LogInfo("  has um lib: {}", SdkCli::hasLibDir(profile, "/um/") ? "yes" : "no");
                std::string probeOutput;
                const bool probeOk = SdkCli::msvcProbe(profile, probeOutput);
                ok &= probeOk;
                LogInfo("  runtime_link_probe: {}", probeOk ? "ok" : "failed");
                if (!probeOk && !probeOutput.empty())
                {
                    LogInfo("  runtime_link_probe_output: {}", probeOutput);
                }
            }
            else if (profile.platform == "android" || profile.sdkKind == "android-ndk")
            {
                std::string probeOutput;
                const bool probeOk = SdkCli::androidProbe(profile, probeOutput);
                ok &= probeOk;
                LogInfo("  compile_probe: {}", probeOk ? "ok" : "failed");
                if (!probeOk && !probeOutput.empty())
                {
                    LogInfo("  compile_probe_output: {}", probeOutput);
                }
            }
        }
        if (!anyAndroidProfile)
        {
            LogInfo("Android SDK candidates:");
            for (const auto &candidate : SdkCli::androidCandidates())
            {
                LogInfo("  {} exists={}", candidate.string(), File::dir(candidate) ? "yes" : "no");
            }
            if (SdkCli::hasAndroidSignal())
            {
                LogInfo("Android SDK found, but Android NDK was not found or no Android toolchain profile is cached.");
                LogInfo("run: rhodium sdk list --refresh");
            }
        }
        return ok;
    }

    int Commands::sdkDetect(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        std::vector<ToolchainProfile> profiles;
        if (!detectAndWriteHostToolchains(workspaceRoot, profiles, diagnostics))
        {
            return 1;
        }
        LogInfo("detected toolchains: {}", profiles.size());
        LogInfo("cache: {}", hostToolchainCachePath(workspaceRoot).string());
        for (const auto &profile : profiles)
        {
            SdkCli::print(profile);
        }
        return profiles.empty() ? 1 : 0;
    }

    int Commands::sdkList(const std::filesystem::path &workspaceRoot, bool refresh, const std::string &sourceFilter = {})
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        LocalToolchainConfig localConfig;
        if (!readLocalToolchainConfig(workspaceRoot, localConfig, diagnostics))
        {
            return 1;
        }
        if (refresh)
        {
            std::vector<ToolchainProfile> refreshed;
            if (!detectAndWriteHostToolchains(workspaceRoot, refreshed, diagnostics))
            {
                return 1;
            }
        }
        auto loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
        if (loadedProfiles.empty())
        {
            std::vector<ToolchainProfile> refreshed;
            if (!detectAndWriteHostToolchains(workspaceRoot, refreshed, diagnostics))
            {
                return 1;
            }
            loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
        }
        if (!sourceFilter.empty())
        {
            ToolchainSource source;
            if (!parseToolchainSource(sourceFilter, source))
            {
                diagnostics.error("unknown toolchain source filter: " + sourceFilter);
                return 1;
            }
            loadedProfiles.erase(std::remove_if(loadedProfiles.begin(), loadedProfiles.end(), [&](const auto &profile)
                                                { return profile.source != source; }),
                                 loadedProfiles.end());
        }
        if (loadedProfiles.empty())
        {
            LogInfo("toolchains: none");
            return 1;
        }
        const auto profiles = unwrapToolchainProfiles(loadedProfiles);
        if (!refresh && !SdkCli::hasAndroidProfile(profiles) && SdkCli::hasAndroidSignal())
        {
            diagnostics.warning("Android SDK appears to be installed but no android toolchain is cached.\nrun:\n  rhodium sdk list --refresh\nor:\n  rhodium sdk detect");
        }
        std::size_t idWidth = std::string("id").size();
        std::size_t platformWidth = std::string("platform").size();
        std::size_t hostWidth = std::string("host").size();
        std::size_t targetWidth = std::string("target").size();
        std::size_t abiWidth = std::string("abi").size();
        std::size_t apiWidth = std::string("api").size();
        std::size_t sourceWidth = std::string("source").size();
        for (const auto &loaded : loadedProfiles)
        {
            const auto &profile = loaded.profile;
            idWidth = (std::max)(idWidth, profile.id.size());
            platformWidth = (std::max)(platformWidth, profile.platform.empty() ? std::size_t{1} : profile.platform.size());
            hostWidth = (std::max)(hostWidth, profile.hostArch.empty() ? std::size_t{1} : profile.hostArch.size());
            targetWidth = (std::max)(targetWidth, profile.targetArch.empty() ? std::size_t{1} : profile.targetArch.size());
            abiWidth = (std::max)(abiWidth, profile.androidAbi.empty() ? std::size_t{1} : profile.androidAbi.size());
            apiWidth = (std::max)(apiWidth, profile.androidApi.empty() ? std::size_t{1} : profile.androidApi.size());
            sourceWidth = (std::max)(sourceWidth, std::string(toString(loaded.source)).size());
        }

        LogInfo("{:<2}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  status",
                "",
                "id", idWidth,
                "platform", platformWidth,
                "host", hostWidth,
                "target", targetWidth,
                "abi", abiWidth,
                "api", apiWidth,
                "source", sourceWidth);
        for (const auto &loaded : loadedProfiles)
        {
            const auto &profile = loaded.profile;
            const auto selected = profile.id == localConfig.preferredToolchain ? "*" : "";
            LogInfo("{:<2}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {:<{}}  {}",
                    selected,
                    profile.id,
                    idWidth,
                    profile.platform.empty() ? "-" : profile.platform,
                    platformWidth,
                    profile.hostArch.empty() ? "-" : profile.hostArch,
                    hostWidth,
                    profile.targetArch.empty() ? "-" : profile.targetArch,
                    targetWidth,
                    profile.androidAbi.empty() ? "-" : profile.androidAbi,
                    abiWidth,
                    profile.androidApi.empty() ? "-" : profile.androidApi,
                    apiWidth,
                    toString(loaded.source),
                    sourceWidth,
                    profile.complete ? "complete" : "incomplete");
            if (!profile.complete && !profile.missing.empty())
            {
                LogInfo("    missing: {}", SdkCli::join(profile.missing, ", "));
            }
        }
        return 0;
    }

    int Commands::sdkShow(const std::filesystem::path &workspaceRoot, const std::string &id)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        const auto profiles = loadMergedToolchains(workspaceRoot, diagnostics);
        const auto found = std::find_if(profiles.begin(), profiles.end(), [&](const LoadedToolchainProfile &profile)
                                        { return profile.profile.id == id; });
        if (found == profiles.end())
        {
            diagnostics.error("toolchain not found: " + id);
            return 1;
        }
        const auto &profile = found->profile;
        LogInfo("id: {}", profile.id);
        LogInfo("source: {}", toString(found->source));
        LogInfo("source_file: {}", found->sourceFile.string());
        LogInfo("platform: {}", profile.platform.empty() ? "-" : profile.platform);
        LogInfo("compiler_kind: {}", profile.compilerKind.empty() ? "-" : profile.compilerKind);
        LogInfo("sdk_kind: {}", profile.sdkKind.empty() ? "-" : profile.sdkKind);
        LogInfo("host_arch: {}", profile.hostArch.empty() ? "-" : profile.hostArch);
        LogInfo("target_arch: {}", profile.targetArch.empty() ? "-" : profile.targetArch);
        LogInfo("target_triple: {}", profile.targetTriple.empty() ? "-" : profile.targetTriple);
        LogInfo("compiler: {}", profile.compiler.empty() ? "-" : profile.compiler.string());
        LogInfo("cxx_compiler: {}", profile.cxxCompiler.empty() ? "-" : profile.cxxCompiler.string());
        LogInfo("linker: {}", profile.linker.empty() ? "-" : profile.linker.string());
        LogInfo("archiver: {}", profile.archiver.empty() ? "-" : profile.archiver.string());
        LogInfo("resource_compiler: {}", profile.resourceCompiler.empty() ? "-" : profile.resourceCompiler.string());
        LogInfo("manifest_tool: {}", profile.manifestTool.empty() ? "-" : profile.manifestTool.string());
        LogInfo("ninja: {}", profile.ninja.empty() ? "-" : profile.ninja.string());
        LogInfo("git: {}", profile.git.empty() ? "-" : profile.git.string());
        LogInfo("android_sdk_root: {}", profile.androidSdkRoot.empty() ? "-" : profile.androidSdkRoot.string());
        LogInfo("android_ndk_root: {}", profile.androidNdkRoot.empty() ? "-" : profile.androidNdkRoot.string());
        LogInfo("jdk_root: {}", profile.jdkRoot.empty() ? "-" : profile.jdkRoot.string());
        LogInfo("android_api: {}", profile.androidApi.empty() ? "-" : profile.androidApi);
        LogInfo("android_abi: {}", profile.androidAbi.empty() ? "-" : profile.androidAbi);
        SdkCli::printPaths("system_include_dirs", profile.systemIncludeDirs);
        SdkCli::printPaths("system_lib_dirs", profile.systemLibDirs);
        SdkCli::printPaths("binary_dirs", profile.binaryDirs);
        LogInfo("environment:");
        if (profile.environment.empty())
        {
            LogInfo("  -");
        }
        for (const auto &[key, value] : profile.environment)
        {
            LogInfo("  {}={}", key, value);
        }
        LogInfo("status: {}", profile.complete ? "complete" : "incomplete");
        if (!profile.missing.empty())
        {
            LogInfo("missing: {}", SdkCli::join(profile.missing, ", "));
        }
        if (!found->overridden.empty())
        {
            LogInfo("overrides:");
            for (const auto &overridden : found->overridden)
            {
                LogInfo("  {}: {}", toString(overridden.source), overridden.sourceFile.string());
            }
        }
        return 0;
    }

    int Commands::sdkAdd(const std::filesystem::path &workspaceRoot, const SdkAddOptions &options)
    {
        ConsoleDiagnosticSink diagnostics;
        ToolchainSource source;
        if (!parseToolchainSource(options.source, source) || (source != ToolchainSource::ProjectUser && source != ToolchainSource::GlobalUser))
        {
            diagnostics.error("sdk add --source must be project-user or global-user");
            return 1;
        }
        auto profile = options.profile;
        if (!SdkCli::applyEnv(profile, options.env, diagnostics))
        {
            return 1;
        }
        if (profile.id.empty())
        {
            diagnostics.error("sdk add requires toolchain id");
            return 1;
        }
        if (profile.cxxCompiler.empty())
        {
            profile.cxxCompiler = profile.compiler;
        }
        if (profile.linker.empty())
        {
            profile.linker = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        }
        if (profile.ninja.empty())
        {
            profile.ninja = findExecutableOnPath("ninja");
        }
        if (profile.git.empty())
        {
            profile.git = findExecutableOnPath("git");
        }
        const auto mutationRoot = source == ToolchainSource::GlobalUser
                                      ? globalUserToolchainsPath().parent_path().parent_path()
                                      : workspaceRoot;
        auto workspaceLock = WorkspaceLock::acquire(mutationRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        std::vector<ToolchainProfile> profiles;
        if (!readUserToolchainProfiles(workspaceRoot, source, profiles, diagnostics))
        {
            return 1;
        }
        const auto existing = std::find_if(profiles.begin(), profiles.end(), [&](const ToolchainProfile &item)
                                           { return item.id == profile.id; });
        if (existing != profiles.end() && !options.force)
        {
            diagnostics.error("toolchain '" + profile.id + "' already exists in " + std::string(toString(source)) + " toolchains.\nuse --force to replace it.");
            return 1;
        }
        if (existing != profiles.end())
        {
            *existing = std::move(profile);
        }
        else
        {
            profiles.push_back(std::move(profile));
        }
        if (!writeUserToolchainProfiles(workspaceRoot, source, profiles, diagnostics))
        {
            return 1;
        }
        diagnostics.info("wrote " + std::string(toString(source)) + " toolchain: " + options.profile.id);
        return 0;
    }

    int Commands::sdkRemove(const std::filesystem::path &workspaceRoot, const std::string &id, const std::string &sourceName)
    {
        ConsoleDiagnosticSink diagnostics;
        ToolchainSource source;
        if (!parseToolchainSource(sourceName.empty() ? "project-user" : sourceName, source) ||
            (source != ToolchainSource::ProjectUser && source != ToolchainSource::GlobalUser))
        {
            diagnostics.error("sdk remove --source must be project-user or global-user");
            return 1;
        }
        const auto mutationRoot = source == ToolchainSource::GlobalUser
                                      ? globalUserToolchainsPath().parent_path().parent_path()
                                      : workspaceRoot;
        auto workspaceLock = WorkspaceLock::acquire(mutationRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        std::vector<ToolchainProfile> profiles;
        if (!readUserToolchainProfiles(workspaceRoot, source, profiles, diagnostics))
        {
            return 1;
        }
        const auto oldSize = profiles.size();
        profiles.erase(std::remove_if(profiles.begin(), profiles.end(), [&](const ToolchainProfile &profile)
                                      { return profile.id == id; }),
                       profiles.end());
        if (profiles.size() == oldSize)
        {
            if ((sourceName.empty() || sourceName == "project-user") && source == ToolchainSource::ProjectUser)
            {
                std::vector<ToolchainProfile> globalProfiles;
                if (readUserToolchainProfiles(workspaceRoot, ToolchainSource::GlobalUser, globalProfiles, diagnostics) &&
                    std::any_of(globalProfiles.begin(), globalProfiles.end(), [&](const ToolchainProfile &profile)
                                { return profile.id == id; }))
                {
                    diagnostics.error("toolchain '" + id + "' is in global-user toolchains; use --source global-user");
                    return 1;
                }
            }
            diagnostics.error("toolchain '" + id + "' was not found in " + std::string(toString(source)) + " toolchains");
            return 1;
        }
        if (!writeUserToolchainProfiles(workspaceRoot, source, profiles, diagnostics))
        {
            return 1;
        }
        diagnostics.info("removed " + std::string(toString(source)) + " toolchain: " + id);
        return 0;
    }

    int Commands::sdkDump(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        std::vector<ToolchainProfile> profiles;
        if (!readToolchainProfiles(hostToolchainCachePath(workspaceRoot), profiles, diagnostics) || profiles.empty())
        {
            if (!detectAndWriteHostToolchains(workspaceRoot, profiles, diagnostics))
            {
                return 1;
            }
        }
        LogInfo("{}", File::read(hostToolchainCachePath(workspaceRoot)));
        return 0;
    }

    int Commands::sdkSelect(const std::filesystem::path &workspaceRoot, const std::string &toolchainId)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        if (toolchainId.empty())
        {
            diagnostics.error("sdk select requires a toolchain id");
            return 1;
        }

        auto loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
        auto profiles = unwrapToolchainProfiles(loadedProfiles);
        auto found = std::find_if(profiles.begin(), profiles.end(), [&](const ToolchainProfile &profile)
                                  { return profile.id == toolchainId; });
        if (found == profiles.end())
        {
            std::vector<ToolchainProfile> refreshed;
            if (!detectAndWriteHostToolchains(workspaceRoot, refreshed, diagnostics))
            {
                return 1;
            }
            loadedProfiles = loadMergedToolchains(workspaceRoot, diagnostics);
            profiles = unwrapToolchainProfiles(loadedProfiles);
            found = std::find_if(profiles.begin(), profiles.end(), [&](const ToolchainProfile &profile)
                                 { return profile.id == toolchainId; });
        }
        if (found == profiles.end())
        {
            diagnostics.error("requested toolchain was not found: " + toolchainId +
                              "\nrun:\n  rhodium sdk list --refresh");
            return 1;
        }

        LocalToolchainConfig config;
        if (!readLocalToolchainConfig(workspaceRoot, config, diagnostics))
        {
            return 1;
        }
        config.preferredToolchain = toolchainId;
        if (!writeLocalToolchainConfig(workspaceRoot, config, diagnostics))
        {
            return 1;
        }
        diagnostics.info("saved preferred toolchain: " + toolchainId);
        return 0;
    }

    int Commands::sdkClear(const std::filesystem::path &workspaceRoot)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics, true);
        if (!workspaceLock)
        {
            return 1;
        }
        LocalToolchainConfig config;
        if (!readLocalToolchainConfig(workspaceRoot, config, diagnostics))
        {
            return 1;
        }
        config.preferredToolchain.clear();
        if (!writeLocalToolchainConfig(workspaceRoot, config, diagnostics))
        {
            return 1;
        }
        diagnostics.info("cleared preferred toolchain");
        return 0;
    }

} // namespace rhodium
