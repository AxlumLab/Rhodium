/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/toolchain/ToolchainProbe.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace rhodium
{
    struct AndroidAbi
    {
        const char *id;
        const char *abi;
        const char *arch;
        const char *toolPrefix;
        const char *targetTriple;
    };

    class AndroidTc
    {
    public:
        static void addRoots(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &sdkRoot,
                             const std::filesystem::path &ndkRoot, DiagnosticSink &diagnostics);
        static void add(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &workspaceRoot,
                        DiagnosticSink &diagnostics);

    private:
        static std::filesystem::path props(const std::filesystem::path &workspaceRoot);
        static std::filesystem::path sdk(const std::filesystem::path &workspaceRoot = {});
        static bool ndkLike(const std::filesystem::path &path);
        static std::filesystem::path ndk(const std::filesystem::path &sdkRoot);
        static std::string api(const std::filesystem::path &sdkRoot, const std::filesystem::path &ndkRoot);
        static std::filesystem::path javaHome();
        static ToolchainProfile profile(const std::filesystem::path &sdkRoot, const std::filesystem::path &ndkRoot,
                                        const std::filesystem::path &prebuilt, const AndroidAbi &abi,
                                        const std::string &api);
    };

    std::filesystem::path AndroidTc::props(const std::filesystem::path &workspaceRoot)
    {
        if (workspaceRoot.empty())
        {
            return {};
        }
        const std::vector<std::filesystem::path> files = {
            workspaceRoot / "local.properties",
            workspaceRoot / "generated" / "android-studio" / "local.properties",
            workspaceRoot / "android" / "local.properties",
        };
        for (const auto &file : files)
        {
            std::ifstream input(file);
            if (!input)
            {
                continue;
            }
            std::string line;
            while (std::getline(input, line))
            {
                constexpr const char *prefix = "sdk.dir=";
                if (line.rfind(prefix, 0) == 0)
                {
                    auto value = line.substr(std::string(prefix).size());
                    std::replace(value.begin(), value.end(), '\\', '/');
                    return value.empty() ? std::filesystem::path{} : std::filesystem::path(value);
                }
            }
        }
        return {};
    }

    std::filesystem::path AndroidTc::sdk(const std::filesystem::path &workspaceRoot)
    {
        std::vector<std::filesystem::path> candidates;
        ToolProbe::appendUnique(candidates, props(workspaceRoot));
        ToolProbe::appendEnvCandidate(candidates, "ANDROID_HOME");
        ToolProbe::appendEnvCandidate(candidates, "ANDROID_SDK_ROOT");
        const auto home = ToolProbe::home();
#ifdef _WIN32
        const auto localAppData = ToolProbe::env("LOCALAPPDATA");
        if (!localAppData.empty())
        {
            ToolProbe::appendUnique(candidates, std::filesystem::path(localAppData) / "Android" / "Sdk");
        }
        const auto userProfile = ToolProbe::env("USERPROFILE");
        if (!userProfile.empty())
        {
            ToolProbe::appendUnique(candidates,
                                    std::filesystem::path(userProfile) / "AppData" / "Local" / "Android" / "Sdk");
        }
        ToolProbe::appendUnique(candidates, "C:/Android/Sdk");
        ToolProbe::appendUnique(candidates, "D:/Android/Sdk");
        ToolProbe::appendUnique(candidates, "D:/Sdks/Android/Sdk");
        ToolProbe::appendUnique(candidates, "C:/Applications/Android/Sdk");
#else
        if (!home.empty())
        {
            if (Environment::hostPlatformName() == "macos")
            {
                ToolProbe::appendUnique(candidates, home / "Library" / "Android" / "sdk");
            }
            ToolProbe::appendUnique(candidates, home / "Android" / "Sdk");
            ToolProbe::appendUnique(candidates, home / "Android" / "sdk");
        }
        if (Environment::hostPlatformName() != "macos")
        {
            ToolProbe::appendUnique(candidates, "/opt/android-sdk");
        }
#endif
        for (const auto &candidate : candidates)
        {
            if (!std::filesystem::is_directory(candidate))
            {
                continue;
            }
            const bool hasAdb = !ToolProbe::inDir(candidate / "platform-tools", "adb").empty();
            const bool hasSdkManager =
                !ToolProbe::inDir(candidate / "cmdline-tools" / "latest" / "bin", "sdkmanager").empty() ||
                !ToolProbe::inDir(candidate / "tools" / "bin", "sdkmanager").empty();
            bool hasPlatform = false;
            const auto platforms = candidate / "platforms";
            if (std::filesystem::is_directory(platforms))
            {
                for (const auto &entry : std::filesystem::directory_iterator(platforms))
                {
                    if (entry.is_directory() && entry.path().filename().string().find("android-") == 0)
                    {
                        hasPlatform = true;
                        break;
                    }
                }
            }
            const bool hasNdk = std::filesystem::is_directory(candidate / "ndk") ||
                                std::filesystem::is_directory(candidate / "ndk-bundle");
            if (hasAdb || hasSdkManager || hasPlatform || hasNdk)
            {
                return candidate;
            }
        }
        return {};
    }

    bool AndroidTc::ndkLike(const std::filesystem::path &path)
    {
        return !path.empty() && std::filesystem::is_directory(path / "toolchains" / "llvm" / "prebuilt");
    }

    std::filesystem::path AndroidTc::ndk(const std::filesystem::path &sdkRoot)
    {
        std::vector<std::filesystem::path> candidates;
        std::vector<std::filesystem::path> envCandidates;
        ToolProbe::appendEnvCandidate(envCandidates, "ANDROID_NDK_HOME");
        ToolProbe::appendEnvCandidate(envCandidates, "ANDROID_NDK_ROOT");
        ToolProbe::appendEnvCandidate(envCandidates, "NDK_ROOT");
        for (const auto &candidate : envCandidates)
        {
            if (std::filesystem::is_directory(candidate))
            {
                return candidate;
            }
        }
        candidates.insert(candidates.end(), envCandidates.begin(), envCandidates.end());
        if (!sdkRoot.empty())
        {
            const auto ndkRoot = sdkRoot / "ndk";
            if (std::filesystem::is_directory(ndkRoot))
            {
                std::vector<std::filesystem::path> ndkVersions;
                for (const auto &entry : std::filesystem::directory_iterator(ndkRoot))
                {
                    if (entry.is_directory() && ndkLike(entry.path()))
                    {
                        ndkVersions.push_back(entry.path());
                    }
                }
                std::sort(ndkVersions.begin(), ndkVersions.end(),
                          [](const auto &lhs, const auto &rhs)
                          {
                              return ToolProbe::newer(lhs.filename().string(), rhs.filename().string());
                          });
                for (const auto &version : ndkVersions)
                {
                    candidates.push_back(version);
                }
            }
            candidates.push_back(sdkRoot / "ndk-bundle");
        }
        for (const auto &candidate : candidates)
        {
            if (ndkLike(candidate))
            {
                return candidate;
            }
        }
        return {};
    }

    std::string AndroidTc::api(const std::filesystem::path &sdkRoot, const std::filesystem::path &ndkRoot)
    {
        (void)ndkRoot;
        const auto rhodiumApi = ToolProbe::env("RHODIUM_ANDROID_API");
        if (!rhodiumApi.empty())
        {
            return rhodiumApi;
        }
        const auto androidApi = ToolProbe::env("ANDROID_API");
        if (!androidApi.empty())
        {
            return androidApi;
        }

        int best = 0;
        const auto platforms = sdkRoot / "platforms";
        if (std::filesystem::is_directory(platforms))
        {
            for (const auto &entry : std::filesystem::directory_iterator(platforms))
            {
                if (!entry.is_directory())
                {
                    continue;
                }
                const auto name = entry.path().filename().string();
                constexpr const char *prefix = "android-";
                if (name.find(prefix) != 0)
                {
                    continue;
                }
                const auto version = name.substr(std::string(prefix).size());
                if (ToolProbe::intAtLeast(version, 1))
                {
                    best = (std::max)(best, std::stoi(version));
                }
            }
        }
        return std::to_string((std::max)(best, 23));
    }

    std::filesystem::path AndroidTc::javaHome()
    {
        std::vector<std::filesystem::path> candidates;
        ToolProbe::appendEnvCandidate(candidates, "JAVA_HOME");
#ifdef _WIN32
        ToolProbe::appendUnique(candidates, "C:/Applications/AndroidStudio/jbr");
        ToolProbe::appendUnique(candidates, "C:/Program Files/Android/Android Studio/jbr");
        ToolProbe::appendUnique(candidates, "C:/Program Files/Android/Android Studio/jre");
        const auto localAppData = ToolProbe::env("LOCALAPPDATA");
        if (!localAppData.empty())
        {
            ToolProbe::appendUnique(candidates,
                                    std::filesystem::path(localAppData) / "Programs" / "Android Studio" / "jbr");
        }
        for (const auto &root : {ToolProbe::programFiles() / "Eclipse Adoptium", ToolProbe::programFiles() / "Java"})
        {
            const auto newest = ToolProbe::newest(root);
            if (!newest.empty())
            {
                ToolProbe::appendUnique(candidates, newest);
            }
        }
#else
        if (Environment::hostPlatformName() == "macos")
        {
            ToolProbe::appendUnique(candidates, "/Applications/Android Studio.app/Contents/jbr/Contents/Home");
            const auto newest = ToolProbe::newest("/Library/Java/JavaVirtualMachines");
            if (!newest.empty())
            {
                ToolProbe::appendUnique(candidates, newest / "Contents" / "Home");
            }
        }
        else
        {
            ToolProbe::appendUnique(candidates, "/opt/android-studio/jbr");
            const auto newest = ToolProbe::newest("/usr/lib/jvm");
            if (!newest.empty())
            {
                ToolProbe::appendUnique(candidates, newest);
            }
        }
#endif
        for (const auto &candidate : candidates)
        {
            if (File::exists(candidate / "bin" / ToolProbe::exe("java")) &&
                File::exists(candidate / "bin" / ToolProbe::exe("javac")))
            {
                return candidate;
            }
        }
        return {};
    }

    ToolchainProfile AndroidTc::profile(const std::filesystem::path &sdkRoot, const std::filesystem::path &ndkRoot,
                                        const std::filesystem::path &prebuilt, const AndroidAbi &abi,
                                        const std::string &api)
    {
        const auto bin = prebuilt / "bin";
        const auto compilerBase = std::string(abi.toolPrefix) + api + "-clang";
        const auto cxxBase = std::string(abi.toolPrefix) + api + "-clang++";
        const auto jdkRoot = javaHome();

        ToolchainProfile profile;
        profile.id = abi.id;
        profile.platform = "android";
        profile.hostArch = ToolProbe::arch();
        profile.targetArch = abi.arch;
        profile.compilerKind = "clang";
        profile.sdkKind = "android-ndk";
        profile.compiler = ToolProbe::inDir(bin, compilerBase);
        profile.cxxCompiler = ToolProbe::inDir(bin, cxxBase);
        if (profile.compiler.empty())
        {
            profile.compiler = ToolProbe::inDir(bin, "clang");
        }
        if (profile.cxxCompiler.empty())
        {
            profile.cxxCompiler = ToolProbe::inDir(bin, "clang++");
        }
        profile.linker = profile.cxxCompiler;
        profile.archiver = ToolProbe::inDir(bin, "llvm-ar");
        profile.strip = ToolProbe::inDir(bin, "llvm-strip");
        profile.ranlib = ToolProbe::inDir(bin, "llvm-ranlib");
        profile.sdkRoot = ndkRoot;
        profile.sysroot = prebuilt / "sysroot";
        profile.androidSdkRoot = sdkRoot;
        profile.androidNdkRoot = ndkRoot;
        profile.jdkRoot = jdkRoot;
        profile.androidApi = api;
        profile.androidAbi = abi.abi;
        profile.targetTriple = std::string(abi.targetTriple) + api;
        profile.adb = ToolProbe::inDir(sdkRoot / "platform-tools", "adb");
        profile.java = ToolProbe::resolveExe(ToolProbe::binExe(jdkRoot, "java"), "java");
        profile.javac = ToolProbe::resolveExe(ToolProbe::binExe(jdkRoot, "javac"), "javac");
        profile.gradle = ToolProbe::gradle();
        ToolProbe::appendDir(profile.binaryDirs, bin);
        ToolProbe::appendDir(profile.binaryDirs, sdkRoot / "platform-tools");
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        ToolProbe::complete(profile);
        return profile;
    }

    void AndroidTc::addRoots(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &sdkRoot,
                             const std::filesystem::path &ndkRoot, DiagnosticSink &diagnostics)
    {
        (void)diagnostics;
        const auto prebuilt = ToolProbe::androidPrebuilt(ndkRoot);
        if (sdkRoot.empty() && ndkRoot.empty())
        {
            return;
        }
        const auto apiLevel = AndroidTc::api(sdkRoot, ndkRoot);
        const AndroidAbi abis[] = {
            {"android-clang-arm64", "arm64-v8a", "arm64", "aarch64-linux-android", "aarch64-linux-android"},
            {"android-clang-armv7", "armeabi-v7a", "armv7", "armv7a-linux-androideabi", "armv7a-linux-androideabi"},
            {"android-clang-x64", "x86_64", "x64", "x86_64-linux-android", "x86_64-linux-android"},
            {"android-clang-x86", "x86", "x86", "i686-linux-android", "i686-linux-android"},
        };
        for (const auto &abi : abis)
        {
            profiles.push_back(profile(sdkRoot, ndkRoot, prebuilt, abi, apiLevel));
        }
    }

    void AndroidTc::add(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &workspaceRoot,
                        DiagnosticSink &diagnostics)
    {
        const auto sdkRoot = sdk(workspaceRoot);
        const auto ndkRoot = ndk(sdkRoot);
        addRoots(profiles, sdkRoot, ndkRoot, diagnostics);
    }

    void ToolchainDetector::android(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &sdkRoot,
                                    const std::filesystem::path &ndkRoot, DiagnosticSink &diagnostics)
    {
        AndroidTc::addRoots(profiles, sdkRoot, ndkRoot, diagnostics);
    }

    void ToolchainDetector::android(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &workspaceRoot,
                                    DiagnosticSink &diagnostics)
    {
        AndroidTc::add(profiles, workspaceRoot, diagnostics);
    }

} // namespace rhodium
