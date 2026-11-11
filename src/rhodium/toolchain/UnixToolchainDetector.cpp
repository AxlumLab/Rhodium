/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/toolchain/ToolchainProbe.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/Process.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rhodium
{
    class UnixTc
    {
    public:
        static void add(std::vector<ToolchainProfile> &profiles);

    private:
        static std::filesystem::path xfind(const std::string &tool);
        static std::filesystem::path sdkPath(const std::string &sdk);
        static std::string sdkVersion(const std::string &sdk);
        static ToolchainProfile apple(const std::string &id,
                                      const std::string &sdkKind,
                                      const std::string &targetArch,
                                      const std::string &targetTriple,
                                      const std::string &deploymentTarget);
        static void addApple(std::vector<ToolchainProfile> &profiles);
        static void addCompilerSearchPaths(ToolchainProfile &profile);
        static void addIncludeSearchPaths(ToolchainProfile &profile);
        static void addLibrarySearchPaths(ToolchainProfile &profile);
        static void addFallbackSearchPaths(ToolchainProfile &profile);
        static std::string dumpMachine(const ToolchainProfile &profile);
        static ToolchainProfile unixProfile(const std::string &compilerKind,
                                            const std::string &cCompiler,
                                            const std::string &cxxCompiler);
    };

    std::filesystem::path UnixTc::xfind(const std::string &tool)
    {
        const auto result = Process::run("xcrun", {"--find", tool});
        if (result.exitCode != 0)
        {
            return {};
        }
        const auto value = ToolProbe::trim(result.output);
        return value.empty() ? std::filesystem::path{} : std::filesystem::path(value);
    }

    std::filesystem::path UnixTc::sdkPath(const std::string &sdk)
    {
        const auto result = Process::run("xcrun", {"--sdk", sdk, "--show-sdk-path"});
        if (result.exitCode != 0)
        {
            return {};
        }
        const auto value = ToolProbe::trim(result.output);
        return value.empty() ? std::filesystem::path{} : std::filesystem::path(value);
    }

    std::string UnixTc::sdkVersion(const std::string &sdk)
    {
        const auto result = Process::run("xcrun", {"--sdk", sdk, "--show-sdk-version"});
        return result.exitCode == 0 ? ToolProbe::trim(result.output) : std::string{};
    }
    ToolchainProfile UnixTc::apple(const std::string &id,
                                   const std::string &sdkKind,
                                   const std::string &targetArch,
                                   const std::string &targetTriple,
                                   const std::string &deploymentTarget)
    {
        ToolchainProfile profile;
        profile.id = id;
        profile.platform = sdkKind == "macosx" ? "macos" : "ios";
        profile.hostArch = ToolProbe::arch();
        profile.targetArch = targetArch;
        profile.compilerKind = "clang";
        profile.sdkKind = sdkKind;
        profile.compiler = xfind("clang");
        profile.cxxCompiler = xfind("clang++");
        profile.linker = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        profile.archiver = xfind("ar");
        profile.libtool = xfind("libtool");
        profile.lipo = xfind("lipo");
        profile.codesign = xfind("codesign");
        profile.sdkRoot = sdkPath(sdkKind);
        profile.sdkVersion = sdkVersion(sdkKind);
        profile.targetTriple = targetTriple;
        profile.deploymentTarget = deploymentTarget;
        ToolProbe::appendDir(profile.binaryDirs, profile.compiler.parent_path());
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        ToolProbe::complete(profile);
        return profile;
    }

    void UnixTc::addApple(std::vector<ToolchainProfile> &profiles)
    {
        if (Environment::hostPlatformName() != "macos")
        {
            return;
        }
        const auto xcodeSelect = Process::run("xcode-select", {"-p"});
        if (xcodeSelect.exitCode != 0)
        {
            return;
        }
        for (auto profile : {
                 apple("macos-clang-x64", "macosx", "x64", "x86_64-apple-macosx", "11.0"),
                 apple("macos-clang-arm64", "macosx", "arm64", "arm64-apple-macosx", "11.0"),
                 apple("ios-clang-arm64", "iphoneos", "arm64", "arm64-apple-ios", "13.0"),
                 apple("ios-simulator-clang-arm64", "iphonesimulator", "arm64", "arm64-apple-ios-simulator", "13.0"),
                 apple("ios-simulator-clang-x64", "iphonesimulator", "x64", "x86_64-apple-ios-simulator", "13.0"),
             })
        {
            if (!profile.compiler.empty() || !profile.cxxCompiler.empty())
            {
                profiles.push_back(std::move(profile));
            }
        }
    }
    void UnixTc::addCompilerSearchPaths(ToolchainProfile &profile)
    {
        addIncludeSearchPaths(profile);
        addLibrarySearchPaths(profile);
        addFallbackSearchPaths(profile);
    }

    void UnixTc::addIncludeSearchPaths(ToolchainProfile &profile)
    {
        const auto compiler = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        if (compiler.empty())
        {
            return;
        }

        const auto result = Process::run(compiler.string(), {"-E", "-x", "c++", "/dev/null", "-v"});
        if (result.exitCode != 0)
        {
            return;
        }

        bool inSearchList = false;
        std::stringstream stream(result.output);
        std::string line;
        while (std::getline(stream, line))
        {
            const auto trimmed = ToolProbe::trim(line);
            if (trimmed == "#include <...> search starts here:")
            {
                inSearchList = true;
                continue;
            }
            if (trimmed == "End of search list.")
            {
                break;
            }
            if (!inSearchList || trimmed.empty())
            {
                continue;
            }

            const auto marker = trimmed.find(" (framework directory)");
            const auto path = marker == std::string::npos ? trimmed : trimmed.substr(0, marker);
            ToolProbe::appendDir(profile.systemIncludeDirs, path);
        }
    }

    void UnixTc::addLibrarySearchPaths(ToolchainProfile &profile)
    {
        const auto compiler = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        if (compiler.empty())
        {
            return;
        }

        const auto result = Process::run(compiler.string(), {"--print-search-dirs"});
        if (result.exitCode != 0)
        {
            return;
        }

        std::stringstream stream(result.output);
        std::string line;
        while (std::getline(stream, line))
        {
            const auto trimmed = ToolProbe::trim(line);
            const auto prefix = std::string("libraries:");
            if (trimmed.size() < prefix.size() || trimmed.substr(0, prefix.size()) != prefix)
            {
                continue;
            }

            auto value = ToolProbe::trim(trimmed.substr(prefix.size()));
            if (!value.empty() && value.front() == '=')
            {
                value.erase(value.begin());
            }
            for (const auto &path : ToolProbe::paths(value))
            {
                ToolProbe::appendDir(profile.systemLibDirs, path);
            }
            return;
        }
    }

    void UnixTc::addFallbackSearchPaths(ToolchainProfile &profile)
    {
        if (profile.systemIncludeDirs.empty())
        {
            ToolProbe::appendDir(profile.systemIncludeDirs, "/usr/local/include");
            ToolProbe::appendDir(profile.systemIncludeDirs, "/usr/include");
        }

        if (!profile.systemLibDirs.empty())
        {
            return;
        }

        const auto machine = dumpMachine(profile);
        if (!machine.empty())
        {
            ToolProbe::appendDir(profile.systemLibDirs, std::filesystem::path("/usr/local/lib") / machine);
            ToolProbe::appendDir(profile.systemLibDirs, std::filesystem::path("/usr/lib") / machine);
            ToolProbe::appendDir(profile.systemLibDirs, std::filesystem::path("/lib") / machine);
        }
        ToolProbe::appendDir(profile.systemLibDirs, "/usr/local/lib64");
        ToolProbe::appendDir(profile.systemLibDirs, "/usr/local/lib");
        ToolProbe::appendDir(profile.systemLibDirs, "/usr/lib64");
        ToolProbe::appendDir(profile.systemLibDirs, "/usr/lib");
        ToolProbe::appendDir(profile.systemLibDirs, "/lib64");
        ToolProbe::appendDir(profile.systemLibDirs, "/lib");
    }

    std::string UnixTc::dumpMachine(const ToolchainProfile &profile)
    {
        const auto compiler = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        if (compiler.empty())
        {
            return {};
        }

        const auto result = Process::run(compiler.string(), {"-dumpmachine"});
        return result.exitCode == 0 ? ToolProbe::trim(result.output) : std::string{};
    }

    ToolchainProfile UnixTc::unixProfile(const std::string &compilerKind,
                                         const std::string &cCompiler,
                                         const std::string &cxxCompiler)
    {
        ToolchainProfile profile;
        profile.id = Environment::hostPlatformName() + "-" + compilerKind + "-x64";
        profile.platform = Environment::hostPlatformName();
        profile.hostArch = ToolProbe::arch();
        profile.targetArch = ToolProbe::arch();
        profile.compilerKind = compilerKind;
        profile.sdkKind = "sysroot";
        profile.compiler = findExecutableOnPath(cCompiler);
        profile.cxxCompiler = findExecutableOnPath(cxxCompiler);
        profile.linker = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        profile.archiver = findExecutableOnPath("ar");
        if (!profile.compiler.empty())
        {
            ToolProbe::appendDir(profile.binaryDirs, profile.compiler.parent_path());
        }
        if (!profile.cxxCompiler.empty())
        {
            ToolProbe::appendDir(profile.binaryDirs, profile.cxxCompiler.parent_path());
        }
        addCompilerSearchPaths(profile);
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        ToolProbe::complete(profile);
        return profile;
    }

    void UnixTc::add(std::vector<ToolchainProfile> &profiles)
    {
        for (auto unix : {
                 unixProfile("clang", "clang", "clang++"),
                 unixProfile("gcc", "gcc", "g++"),
             })
        {
            if (!unix.compiler.empty() || !unix.cxxCompiler.empty())
            {
                profiles.push_back(std::move(unix));
            }
        }
        addApple(profiles);
    }

    void addUnixProfiles(std::vector<ToolchainProfile> &profiles)
    {
        UnixTc::add(profiles);
    }

} // namespace rhodium
