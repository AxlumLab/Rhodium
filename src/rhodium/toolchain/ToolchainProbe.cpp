/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/toolchain/ToolchainProbe.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/StringUtil.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace rhodium
{
    std::vector<std::string> ToolProbe::paths(const std::string &value)
    {
        std::vector<std::string> result;
#ifdef _WIN32
        const char delimiter = ';';
#else
        const char delimiter = ':';
#endif
        std::stringstream stream(value);
        std::string item;
        while (std::getline(stream, item, delimiter))
        {
            if (!item.empty())
            {
                result.push_back(item);
            }
        }
        return result;
    }

    std::string ToolProbe::env(const char *key)
    {
#ifdef _MSC_VER
        char *value = nullptr;
        std::size_t size = 0;
        if (_dupenv_s(&value, &size, key) == 0 && value)
        {
            std::string result(value);
            std::free(value);
            return result;
        }
        std::free(value);
        return {};
#else
        if (const char *value = std::getenv(key))
        {
            return value;
        }
        return {};
#endif
    }

    std::string ToolProbe::trim(std::string value)
    {
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                                [](unsigned char c)
                                                {
                                                    return std::isspace(c) == 0;
                                                }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
                                 [](unsigned char c)
                                 {
                                     return std::isspace(c) == 0;
                                 })
                        .base(),
                    value.end());
        return value;
    }

    std::string ToolProbe::arch()
    {
#if defined(_M_ARM64) || defined(__aarch64__)
        return "arm64";
#elif defined(_M_IX86) || defined(__i386__)
        return "x86";
#else
        return "x64";
#endif
    }

    bool ToolProbe::newer(const std::string &lhs, const std::string &rhs)
    {
        std::size_t li = 0;
        std::size_t ri = 0;
        while (li < lhs.size() || ri < rhs.size())
        {
            while (li < lhs.size() && !std::isdigit(static_cast<unsigned char>(lhs[li])))
            {
                ++li;
            }
            while (ri < rhs.size() && !std::isdigit(static_cast<unsigned char>(rhs[ri])))
            {
                ++ri;
            }
            if (li >= lhs.size() || ri >= rhs.size())
            {
                break;
            }
            std::size_t lend = li;
            std::size_t rend = ri;
            while (lend < lhs.size() && std::isdigit(static_cast<unsigned char>(lhs[lend])))
            {
                ++lend;
            }
            while (rend < rhs.size() && std::isdigit(static_cast<unsigned char>(rhs[rend])))
            {
                ++rend;
            }
            const auto lnum = std::stoll(lhs.substr(li, lend - li));
            const auto rnum = std::stoll(rhs.substr(ri, rend - ri));
            if (lnum != rnum)
            {
                return lnum > rnum;
            }
            li = lend;
            ri = rend;
        }
        return lhs > rhs;
    }

    std::filesystem::path ToolProbe::exe(std::string name)
    {
#ifdef _WIN32
        if (name.size() < 4 || name.substr(name.size() - 4) != ".exe")
        {
            name += ".exe";
        }
#endif
        return std::filesystem::path(name);
    }

    std::vector<std::string> ToolProbe::exeNames(const std::string &baseName)
    {
#ifdef _WIN32
        return {baseName + ".exe", baseName + ".cmd", baseName + ".bat", baseName};
#else
        return {baseName, baseName + ".sh"};
#endif
    }

    std::filesystem::path ToolProbe::inDir(const std::filesystem::path &dir, const std::string &baseName)
    {
        if (dir.empty())
        {
            return {};
        }
        for (const auto &name : ToolProbe::exeNames(baseName))
        {
            const auto path = dir / name;
            if (File::exists(path))
            {
                return path;
            }
        }
        return {};
    }

    void ToolProbe::appendDir(std::vector<std::filesystem::path> &values, const std::filesystem::path &path)
    {
        if (!path.empty() && std::filesystem::is_directory(path) &&
            std::find(values.begin(), values.end(), path) == values.end())
        {
            values.push_back(path);
        }
    }

    std::filesystem::path ToolProbe::newest(const std::filesystem::path &root)
    {
        if (!std::filesystem::is_directory(root))
        {
            return {};
        }
        std::vector<std::filesystem::path> dirs;
        for (const auto &entry : std::filesystem::directory_iterator(root))
        {
            if (entry.is_directory())
            {
                dirs.push_back(entry.path());
            }
        }
        std::sort(dirs.begin(), dirs.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                      return ToolProbe::newer(lhs.filename().string(), rhs.filename().string());
                  });
        return dirs.empty() ? std::filesystem::path{} : dirs.front();
    }

    std::filesystem::path ToolProbe::newestOf(const std::vector<std::filesystem::path> &roots)
    {
        std::vector<std::filesystem::path> dirs;
        for (const auto &root : roots)
        {
            if (std::filesystem::is_directory(root))
            {
                dirs.push_back(root);
            }
        }
        std::sort(dirs.begin(), dirs.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                      return ToolProbe::newer(lhs.filename().string(), rhs.filename().string());
                  });
        return dirs.empty() ? std::filesystem::path{} : dirs.front();
    }

    std::filesystem::path ToolProbe::programFiles()
    {
        auto value = ToolProbe::env("ProgramFiles");
        return value.empty() ? std::filesystem::path("C:/Program Files") : std::filesystem::path(value);
    }

    std::filesystem::path ToolProbe::programFilesX86()
    {
        auto value = ToolProbe::env("ProgramFiles(x86)");
        return value.empty() ? std::filesystem::path("C:/Program Files (x86)") : std::filesystem::path(value);
    }

    void ToolProbe::addTools(ToolchainProfile &profile)
    {
        profile.cmake = ToolchainDetector::findExecutable("cmake");
        profile.ninja = ToolchainDetector::findExecutable("ninja");
        profile.git = ToolchainDetector::findExecutable("git");
    }

    void ToolProbe::appendEnvDir(std::vector<std::filesystem::path> &paths, const char *key)
    {
        const auto value = ToolProbe::env(key);
        if (!value.empty())
        {
            ToolProbe::appendDir(paths, value);
        }
    }

    void ToolProbe::appendEnvCandidate(std::vector<std::filesystem::path> &paths, const char *key)
    {
        const auto value = ToolProbe::env(key);
        if (!value.empty())
        {
            paths.emplace_back(value);
        }
    }

    void ToolProbe::appendUnique(std::vector<std::filesystem::path> &paths, const std::filesystem::path &path)
    {
        if (!path.empty() && std::find(paths.begin(), paths.end(), path) == paths.end())
        {
            paths.push_back(path);
        }
    }

    std::filesystem::path ToolProbe::home()
    {
        auto home = ToolProbe::env("HOME");
        if (!home.empty())
        {
            return home;
        }
        const auto userProfile = ToolProbe::env("USERPROFILE");
        return userProfile.empty() ? std::filesystem::path{} : std::filesystem::path(userProfile);
    }

    bool ToolProbe::intAtLeast(const std::string &value, int minimum)
    {
        if (value.empty())
        {
            return false;
        }
        for (const char c : value)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
            {
                return false;
            }
        }
        try
        {
            return std::stoi(value) >= minimum;
        }
        catch (...)
        {
            return false;
        }
    }

    std::filesystem::path ToolProbe::binExe(const std::filesystem::path &root, const std::string &name)
    {
        return root.empty() ? std::filesystem::path{} : root / "bin" / ToolProbe::exe(name);
    }

    std::filesystem::path ToolProbe::resolveExe(const std::filesystem::path &candidate, const std::string &fallback)
    {
        if (!candidate.empty() && File::exists(candidate))
        {
            return candidate;
        }
        return ToolchainDetector::findExecutable(fallback);
    }

    std::filesystem::path ToolProbe::gradle()
    {
        auto gradle = ToolchainDetector::findExecutable("gradle");
        if (!gradle.empty())
        {
            return gradle;
        }
#ifdef _WIN32
        return ToolchainDetector::findExecutable("gradle.bat");
#else
        return {};
#endif
    }

    void ToolProbe::addPath(ToolchainProfile &profile)
    {
        std::string path;
        for (const auto &dir : profile.binaryDirs)
        {
            if (dir.empty())
            {
                continue;
            }
            if (!path.empty())
            {
#ifdef _WIN32
                path += ';';
#else
                path += ':';
#endif
            }
            path += dir.string();
        }
        const auto currentPath = ToolProbe::env("PATH");
        if (!currentPath.empty())
        {
            if (!path.empty())
            {
#ifdef _WIN32
                path += ';';
#else
                path += ':';
#endif
            }
            path += currentPath;
        }
        if (!path.empty())
        {
            profile.environment["PATH"] = path;
        }
    }

    std::string ToolProbe::join(const std::vector<std::filesystem::path> &paths)
    {
        std::string result;
        for (const auto &path : paths)
        {
            if (path.empty())
            {
                continue;
            }
            if (!result.empty())
            {
#ifdef _WIN32
                result += ';';
#else
                result += ':';
#endif
            }
            result += path.string();
        }
        return result;
    }

    std::filesystem::path ToolProbe::androidPrebuilt(const std::filesystem::path &ndkRoot)
    {
        if (ndkRoot.empty())
        {
            return {};
        }
        std::vector<std::string> tags;
        const auto platform = Environment::hostPlatformName();
        if (platform == "windows")
        {
            tags = {"windows-x86_64"};
        }
        else if (platform == "macos")
        {
            tags = ToolProbe::arch() == "arm64" ? std::vector<std::string>{"darwin-arm64", "darwin-x86_64"}
                                                : std::vector<std::string>{"darwin-x86_64", "darwin-arm64"};
        }
        else
        {
            tags = {"linux-x86_64"};
        }
        for (const auto &tag : tags)
        {
            const auto dir = ndkRoot / "toolchains" / "llvm" / "prebuilt" / tag;
            if (std::filesystem::is_directory(dir))
            {
                return dir;
            }
        }
        return {};
    }

    bool ToolProbe::hasPart(const std::filesystem::path &path, const std::string &part)
    {
        const auto lowerPath = StringUtils::toLower(path.generic_string());
        return lowerPath.find(part) != std::string::npos;
    }

    bool ToolProbe::hasMsvcLib(const ToolchainProfile &profile)
    {
        const auto suffix = "/lib/" + StringUtils::toLower(profile.targetArch);
        return std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(),
                           [](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, "/vc/tools/msvc/") && ToolProbe::hasPart(dir, "/lib/");
                           }) &&
               std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(),
                           [&](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, suffix);
                           });
    }

    bool ToolProbe::hasUcrtLib(const ToolchainProfile &profile)
    {
        const auto suffix = "/ucrt/" + StringUtils::toLower(profile.targetArch);
        return std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(),
                           [](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, "/windows kits/10/lib/");
                           }) &&
               std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(),
                           [&](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, suffix);
                           });
    }

    bool ToolProbe::hasUmLib(const ToolchainProfile &profile)
    {
        const auto suffix = "/um/" + StringUtils::toLower(profile.targetArch);
        return std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(),
                           [](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, "/windows kits/10/lib/");
                           }) &&
               std::any_of(profile.systemLibDirs.begin(), profile.systemLibDirs.end(),
                           [&](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, suffix);
                           });
    }

    bool ToolProbe::hasInclude(const ToolchainProfile &profile, const std::string &part)
    {
        return std::any_of(profile.systemIncludeDirs.begin(), profile.systemIncludeDirs.end(),
                           [&](const auto &dir)
                           {
                               return ToolProbe::hasPart(dir, part);
                           });
    }

    void ToolProbe::androidTools(const ToolchainProfile &profile, std::vector<std::string> &requiredMissing,
                                 std::vector<std::string> &optionalMissing)
    {
        if (profile.compiler.empty() || !File::exists(profile.compiler))
        {
            requiredMissing.push_back("android_clang");
        }
        if (profile.cxxCompiler.empty() || !File::exists(profile.cxxCompiler))
        {
            requiredMissing.push_back("android_clangxx");
        }
        if (profile.archiver.empty() || !File::exists(profile.archiver))
        {
            requiredMissing.push_back("android_llvm_ar");
        }
        if (profile.linker.empty() || !File::exists(profile.linker))
        {
            requiredMissing.push_back("linker");
        }
        if (profile.adb.empty() || !File::exists(profile.adb))
        {
            optionalMissing.push_back("optional:adb");
        }
        if (profile.java.empty() || !File::exists(profile.java))
        {
            optionalMissing.push_back("optional:java");
        }
        if (profile.javac.empty() || !File::exists(profile.javac))
        {
            optionalMissing.push_back("optional:javac");
        }
        if (profile.gradle.empty() || !File::exists(profile.gradle))
        {
            optionalMissing.push_back("optional:gradle");
        }
    }

    void ToolProbe::completeAndroid(ToolchainProfile &profile)
    {
        std::vector<std::string> requiredMissing;
        std::vector<std::string> optionalMissing;
        const auto prebuilt = ToolProbe::androidPrebuilt(profile.androidNdkRoot);
        const auto sysroot = profile.sysroot.empty()
                                 ? (prebuilt.empty() ? std::filesystem::path{} : prebuilt / "sysroot")
                                 : profile.sysroot;

        if (profile.androidSdkRoot.empty() || !std::filesystem::is_directory(profile.androidSdkRoot))
        {
            requiredMissing.push_back("android_sdk");
        }
        if (profile.androidNdkRoot.empty() || !std::filesystem::is_directory(profile.androidNdkRoot))
        {
            requiredMissing.push_back("android_ndk");
        }
        if (prebuilt.empty() || !std::filesystem::is_directory(prebuilt / "bin"))
        {
            requiredMissing.push_back("android_prebuilt_bin");
        }
        if (sysroot.empty() || !std::filesystem::is_directory(sysroot))
        {
            requiredMissing.push_back("android_sysroot");
        }
        if (!ToolProbe::intAtLeast(profile.androidApi, 1))
        {
            requiredMissing.push_back("android_api");
        }
        if (profile.androidAbi.empty())
        {
            requiredMissing.push_back("android_abi");
        }
        if (profile.targetTriple.empty())
        {
            requiredMissing.push_back("target_triple");
        }
        androidTools(profile, requiredMissing, optionalMissing);
        profile.missing = requiredMissing;
        profile.missing.insert(profile.missing.end(), optionalMissing.begin(), optionalMissing.end());
        profile.complete = requiredMissing.empty();
    }

    void ToolProbe::completeDesktop(ToolchainProfile &profile, bool usesSdkSysroot)
    {
        if (!File::exists(profile.compiler) && !File::exists(profile.cxxCompiler))
        {
            profile.missing.push_back("compiler");
        }
        if (profile.linker.empty() || !File::exists(profile.linker))
        {
            profile.missing.push_back("linker");
        }
        if (profile.archiver.empty() || !File::exists(profile.archiver))
        {
            profile.missing.push_back("archiver");
        }
        if (!usesSdkSysroot && profile.systemIncludeDirs.empty())
        {
            profile.missing.push_back("include_dirs");
        }
        if (!usesSdkSysroot && profile.systemLibDirs.empty())
        {
            profile.missing.push_back("lib_dirs");
        }
        if (usesSdkSysroot && profile.sdkRoot.empty() && profile.sysroot.empty())
        {
            profile.missing.push_back("sdk_root");
        }
    }

    void ToolProbe::completeWindows(ToolchainProfile &profile)
    {
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") && profile.sdkKind != "windows-sdk")
        {
            profile.missing.push_back("windows_sdk");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (profile.compiler.empty() || !File::exists(profile.compiler)))
        {
            profile.missing.push_back("msvc_cl");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (profile.linker.empty() || !File::exists(profile.linker)))
        {
            profile.missing.push_back("msvc_link");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (profile.archiver.empty() || !File::exists(profile.archiver)))
        {
            profile.missing.push_back("msvc_lib");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            !ToolProbe::hasInclude(profile, "/vc/tools/msvc/"))
        {
            profile.missing.push_back("msvc_include");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") && !ToolProbe::hasMsvcLib(profile))
        {
            profile.missing.push_back("msvc_lib_" + profile.targetArch);
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            !ToolProbe::hasInclude(profile, "/ucrt"))
        {
            profile.missing.push_back("windows_sdk_ucrt_include");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            !ToolProbe::hasInclude(profile, "/um"))
        {
            profile.missing.push_back("windows_sdk_um_include");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            !ToolProbe::hasInclude(profile, "/shared"))
        {
            profile.missing.push_back("windows_sdk_shared_include");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") && !ToolProbe::hasUcrtLib(profile))
        {
            profile.missing.push_back("windows_sdk_ucrt_lib_" + profile.targetArch);
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") && !ToolProbe::hasUmLib(profile))
        {
            profile.missing.push_back("windows_sdk_um_lib_" + profile.targetArch);
        }
        auto includeEnv = profile.environment.find("INCLUDE");
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (includeEnv == profile.environment.end() || includeEnv->second.empty()))
        {
            profile.missing.push_back("environment_INCLUDE");
        }
        auto libEnv = profile.environment.find("LIB");
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (libEnv == profile.environment.end() || libEnv->second.empty()))
        {
            profile.missing.push_back("environment_LIB");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (profile.resourceCompiler.empty() || !File::exists(profile.resourceCompiler)))
        {
            profile.missing.push_back("rc");
        }
        if ((profile.compilerKind == "msvc" || profile.compilerKind == "clang-cl") &&
            (profile.manifestTool.empty() || !File::exists(profile.manifestTool)))
        {
            profile.missing.push_back("mt");
        }
    }

    void ToolProbe::complete(ToolchainProfile &profile)
    {
        profile.missing.clear();
        const bool usesSdkSysroot = profile.sdkKind == "macosx" || profile.sdkKind == "iphoneos" ||
                                    profile.sdkKind == "iphonesimulator" || profile.sdkKind == "android-ndk";
        if (profile.sdkKind == "android-ndk")
        {
            completeAndroid(profile);
            return;
        }
        completeDesktop(profile, usesSdkSysroot);
        completeWindows(profile);
        profile.complete = profile.missing.empty();
    }

    std::filesystem::path ToolchainDetector::findTool(const std::filesystem::path &dir, const std::string &baseName)
    {
        return ToolProbe::inDir(dir, baseName);
    }

    std::filesystem::path ToolchainDetector::findExecutable(const std::string &name)
    {
        const auto executable = ToolProbe::exe(name);
        auto direct = std::filesystem::path(name);
        if ((direct.is_absolute() || direct.has_parent_path()) && File::exists(direct))
        {
            return direct;
        }

        for (const auto &dir : ToolProbe::paths(ToolProbe::env("PATH")))
        {
            auto candidate = std::filesystem::path(dir) / executable;
            if (File::exists(candidate))
            {
                return candidate;
            }
        }
        return {};
    }

} // namespace rhodium
