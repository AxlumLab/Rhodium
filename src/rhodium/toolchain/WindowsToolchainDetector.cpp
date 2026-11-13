/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/toolchain/ToolchainProbe.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rhodium
{
    struct WindowsTargetArch
    {
        const char *idSuffix;
        const char *targetArch;
        const char *binArch;
    };

    constexpr WindowsTargetArch kWindowsTargetArchs[] = {
        {"x64", "x64", "x64"},
        {"x86", "x86", "x86"},
        {"arm64", "arm64", "arm64"},
    };

    class WinTc
    {
    public:
        static void add(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
        static void addMsvcRoots(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &vsRoot,
                                 const std::filesystem::path &windowsSdkRoot, DiagnosticSink &diagnostics);

    private:
        static std::filesystem::path vswhere();
        static std::vector<std::filesystem::path> vsRoots();
        static std::filesystem::path trimTrailingSeparator(std::filesystem::path path);
        static std::filesystem::path reg(const std::string &key, const std::string &valueName);
        static std::vector<std::pair<std::string, std::filesystem::path>> regPaths(const std::string &key);
        static std::filesystem::path vsRootFromCompiler(const std::filesystem::path &compiler);
        static std::filesystem::path sdkRoot(const std::map<std::string, std::string> &env = {});
        static std::filesystem::path sdkVersionDir(const std::filesystem::path &sdkRoot);
        static void addSdkRoot(ToolchainProfile &profile, const std::filesystem::path &sdkRoot);
        static void addSdk(ToolchainProfile &profile, const std::map<std::string, std::string> &env = {});
        static void env(ToolchainProfile &profile);
        static std::map<std::string, std::string> parseEnv(const std::string &text);
        static std::map<std::string, std::string> vsEnv(const std::filesystem::path &vsRoot);
        static std::filesystem::path latestMsvc();
        static void addRuntime(ToolchainProfile &profile);
        static ToolchainProfile clangCl(const std::filesystem::path &compiler, const std::filesystem::path &binRoot,
                                        const std::filesystem::path &toolRoot, const std::filesystem::path &sdkRoot,
                                        const WindowsTargetArch &target);
        static void addClangCl(std::vector<ToolchainProfile> &profiles);
        static ToolchainProfile gcc();
        static ToolchainProfile clangGnu();
        static ToolchainProfile msvc(const std::filesystem::path &vsRoot, const std::filesystem::path &toolRoot,
                                     const std::filesystem::path &sdkRoot,
                                     const std::map<std::string, std::string> &vsEnv, const WindowsTargetArch &target);
        static void addMsvc(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics);
    };

    std::filesystem::path WinTc::vswhere()
    {
        auto path = ToolchainDetector::findExecutable("vswhere");
        if (!path.empty())
        {
            return path;
        }
        auto standard = ToolProbe::programFilesX86() / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
        return File::exists(standard) ? standard : std::filesystem::path{};
    }

    std::filesystem::path WinTc::trimTrailingSeparator(std::filesystem::path path)
    {
        while (!path.empty() && path.filename().empty())
        {
            path = path.parent_path();
        }
        return path;
    }

    std::vector<std::filesystem::path> WinTc::vsRoots()
    {
        std::vector<std::filesystem::path> roots;
        const auto addRoot = [&roots](const std::filesystem::path &path)
        {
            if (std::filesystem::is_directory(path))
            {
                ToolProbe::appendUnique(roots, path);
            }
        };

        addRoot(ToolProbe::env("VSINSTALLDIR"));
        const auto vcInstallDir = trimTrailingSeparator(ToolProbe::env("VCINSTALLDIR"));
        if (!vcInstallDir.empty())
        {
            addRoot(vcInstallDir.parent_path());
        }
        auto vcToolsInstallDir = trimTrailingSeparator(ToolProbe::env("VCToolsInstallDir"));
        if (!vcToolsInstallDir.empty())
        {
            addRoot(vcToolsInstallDir.parent_path().parent_path().parent_path().parent_path());
        }
        addRoot(vsRootFromCompiler(ToolchainDetector::findExecutable("cl")));

        const auto vswherePath = WinTc::vswhere();
        if (!vswherePath.empty())
        {
            auto result = Process::run(vswherePath.string(), {"-products", "*", "-requires",
                                                              "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                                              "-property", "installationPath"});
            std::stringstream stream(result.output);
            std::string line;
            while (std::getline(stream, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                addRoot(line);
            }
        }

        for (const auto &key : {
                 std::string("HKLM\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7"),
                 std::string("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\VisualStudio\\SxS\\VS7"),
             })
        {
            for (const auto &[_, path] : regPaths(key))
            {
                addRoot(path);
            }
        }

        for (const auto &base : {
                 ToolProbe::programFiles() / "Microsoft Visual Studio",
                 ToolProbe::programFilesX86() / "Microsoft Visual Studio",
             })
        {
            for (const auto &version : {"2022", "2019", "2017"})
            {
                for (const auto &edition : {"Community", "Professional", "Enterprise", "BuildTools"})
                {
                    addRoot(base / version / edition);
                }
            }
        }
        for (const auto &path : {
                 std::filesystem::path("C:/Applications/Vs"),
                 std::filesystem::path("C:/Applications/Visual Studio"),
             })
        {
            addRoot(path);
        }
        return roots;
    }

    std::filesystem::path WinTc::reg(const std::string &key, const std::string &valueName)
    {
#ifdef _WIN32
        auto result = Process::run("reg", {"query", key, "/v", valueName});
        if (result.exitCode != 0)
        {
            return {};
        }
        std::stringstream stream(result.output);
        std::string line;
        while (std::getline(stream, line))
        {
            if (line.find(valueName) == std::string::npos)
            {
                continue;
            }
            std::stringstream parts(line);
            std::string name;
            std::string type;
            std::string data;
            parts >> name >> type;
            std::getline(parts, data);
            data.erase(data.begin(), std::find_if(data.begin(), data.end(),
                                                  [](unsigned char c)
                                                  {
                                                      return std::isspace(c) == 0;
                                                  }));
            if (!data.empty())
            {
                return data;
            }
        }
#else
        (void)key;
        (void)valueName;
#endif
        return {};
    }

    std::vector<std::pair<std::string, std::filesystem::path>> WinTc::regPaths(const std::string &key)
    {
        std::vector<std::pair<std::string, std::filesystem::path>> paths;
#ifdef _WIN32
        auto result = Process::run("reg", {"query", key});
        if (result.exitCode != 0)
        {
            return paths;
        }
        std::stringstream stream(result.output);
        std::string line;
        while (std::getline(stream, line))
        {
            std::stringstream parts(line);
            std::string name;
            std::string type;
            std::string data;
            parts >> name >> type;
            if (type != "REG_SZ" && type != "REG_EXPAND_SZ")
            {
                continue;
            }
            std::getline(parts, data);
            data.erase(data.begin(), std::find_if(data.begin(), data.end(),
                                                  [](unsigned char c)
                                                  {
                                                      return std::isspace(c) == 0;
                                                  }));
            if (!name.empty() && !data.empty())
            {
                paths.emplace_back(name, data);
            }
        }
#else
        (void)key;
#endif
        return paths;
    }

    std::filesystem::path WinTc::vsRootFromCompiler(const std::filesystem::path &compiler)
    {
        if (compiler.empty() || compiler.filename() != "cl.exe")
        {
            return {};
        }
        const auto targetDir = compiler.parent_path();
        const auto hostDir = targetDir.parent_path();
        const auto binDir = hostDir.parent_path();
        const auto versionDir = binDir.parent_path();
        const auto msvcDir = versionDir.parent_path();
        const auto toolsDir = msvcDir.parent_path();
        const auto vcDir = toolsDir.parent_path();
        const auto vsRoot = vcDir.parent_path();
        if (binDir.filename() == "bin" && msvcDir.filename() == "MSVC" && toolsDir.filename() == "Tools" &&
            vcDir.filename() == "VC" && std::filesystem::is_directory(vsRoot))
        {
            return vsRoot;
        }
        return {};
    }

    std::filesystem::path WinTc::sdkRoot(const std::map<std::string, std::string> &env)
    {
        for (const auto *key : {"WindowsSdkDir", "WindowsSDKDir", "WINDOWSSDKDIR", "KitsRoot10", "WindowsSdkRoot"})
        {
            auto envIt = env.find(key);
            if (envIt != env.end() && std::filesystem::is_directory(envIt->second))
            {
                return envIt->second;
            }
            auto envValue = ToolProbe::env(key);
            if (!envValue.empty() && std::filesystem::is_directory(envValue))
            {
                return envValue;
            }
        }
        for (const auto &key : {
                 std::string("HKLM\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots"),
                 std::string("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots"),
             })
        {
            for (const auto *valueName : {"KitsRoot10", "KitsRoot81", "KitsRoot"})
            {
                const auto value = reg(key, valueName);
                if (!value.empty() && std::filesystem::is_directory(value))
                {
                    return value;
                }
            }
            for (const auto &[name, path] : regPaths(key))
            {
                if (name.rfind("KitsRoot", 0) == 0 && std::filesystem::is_directory(path))
                {
                    return path;
                }
            }
        }
        for (const auto &standard : {
                 ToolProbe::programFilesX86() / "Windows Kits" / "10",
                 ToolProbe::programFiles() / "Windows Kits" / "10",
                 std::filesystem::path("C:/Applications/Windows Kits/10"),
             })
        {
            if (std::filesystem::is_directory(standard))
            {
                return standard;
            }
        }
        return {};
    }

    std::filesystem::path WinTc::sdkVersionDir(const std::filesystem::path &sdkRoot)
    {
        if (sdkRoot.empty())
        {
            return {};
        }
        const auto includeRoot = sdkRoot / "Include";
        if (!std::filesystem::is_directory(includeRoot))
        {
            return {};
        }
        std::vector<std::filesystem::path> candidates;
        for (const auto &entry : std::filesystem::directory_iterator(includeRoot))
        {
            if (!entry.is_directory())
            {
                continue;
            }
            const auto version = entry.path().filename();
            const auto libRoot = sdkRoot / "Lib" / version;
            if (std::filesystem::is_directory(entry.path() / "um") &&
                std::filesystem::is_directory(entry.path() / "ucrt") && std::filesystem::is_directory(libRoot / "um") &&
                std::filesystem::is_directory(libRoot / "ucrt"))
            {
                candidates.push_back(entry.path());
            }
        }
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto &lhs, const auto &rhs)
                  {
                      return lhs.filename().string() > rhs.filename().string();
                  });
        return candidates.empty() ? std::filesystem::path{} : candidates.front();
    }

    void WinTc::addSdkRoot(ToolchainProfile &profile, const std::filesystem::path &sdkRoot)
    {
        const auto versionDir = sdkVersionDir(sdkRoot);
        if (sdkRoot.empty() || versionDir.empty())
        {
            return;
        }

        profile.sdkKind = "windows-sdk";
        profile.sdkRoot = sdkRoot;
        profile.sdkVersion = versionDir.filename().string();
        const auto includeRoot = sdkRoot / "Include" / profile.sdkVersion;
        const auto libRoot = sdkRoot / "Lib" / profile.sdkVersion;
        ToolProbe::appendDir(profile.systemIncludeDirs, includeRoot / "ucrt");
        ToolProbe::appendDir(profile.systemIncludeDirs, includeRoot / "shared");
        ToolProbe::appendDir(profile.systemIncludeDirs, includeRoot / "um");
        ToolProbe::appendDir(profile.systemIncludeDirs, includeRoot / "winrt");
        ToolProbe::appendDir(profile.systemIncludeDirs, includeRoot / "cppwinrt");
        ToolProbe::appendDir(profile.systemLibDirs, libRoot / "ucrt" / profile.targetArch);
        ToolProbe::appendDir(profile.systemLibDirs, libRoot / "um" / profile.targetArch);
        ToolProbe::appendDir(profile.binaryDirs, sdkRoot / "bin" / profile.sdkVersion / profile.hostArch);
        profile.resourceCompiler = (sdkRoot / "bin" / profile.sdkVersion / profile.hostArch / "rc.exe");
        profile.manifestTool = (sdkRoot / "bin" / profile.sdkVersion / profile.hostArch / "mt.exe");
        if (!File::exists(profile.resourceCompiler))
        {
            profile.resourceCompiler = sdkRoot / "bin" / profile.hostArch / "rc.exe";
        }
        if (!File::exists(profile.manifestTool))
        {
            profile.manifestTool = sdkRoot / "bin" / profile.hostArch / "mt.exe";
        }
    }

    void WinTc::addSdk(ToolchainProfile &profile, const std::map<std::string, std::string> &env)
    {
        addSdkRoot(profile, sdkRoot(env));
    }

    void WinTc::env(ToolchainProfile &profile)
    {
        if (profile.compilerKind != "msvc" && profile.compilerKind != "clang-cl")
        {
            return;
        }

        const auto include = ToolProbe::join(profile.systemIncludeDirs);
        const auto lib = ToolProbe::join(profile.systemLibDirs);
        if (!include.empty())
        {
            profile.environment["INCLUDE"] = include;
        }
        if (!lib.empty())
        {
            profile.environment["LIB"] = lib;
        }
        if (!lib.empty())
        {
            profile.environment["LIBPATH"] = lib;
        }
        auto sdkIt = profile.environment.find("WindowsSdkDir");
        if (!profile.sdkRoot.empty() && (sdkIt == profile.environment.end() || sdkIt->second.empty()))
        {
            profile.environment["WindowsSdkDir"] = profile.sdkRoot.string();
        }
        if (profile.environment.find("VCToolsInstallDir") == profile.environment.end())
        {
            for (const auto &includeDir : profile.systemIncludeDirs)
            {
                if (includeDir.filename() == "include")
                {
                    auto toolRoot = includeDir.parent_path();
                    if (File::dir(toolRoot / "lib" / profile.targetArch))
                    {
                        profile.environment["VCToolsInstallDir"] = toolRoot.string();
                        profile.environment["VCINSTALLDIR"] =
                            toolRoot.parent_path().parent_path().parent_path().string();
                        break;
                    }
                }
            }
        }
    }

    std::map<std::string, std::string> WinTc::parseEnv(const std::string &text)
    {
        std::map<std::string, std::string> env;
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            const auto equals = line.find('=');
            if (equals == std::string::npos || equals == 0)
            {
                continue;
            }
            env[line.substr(0, equals)] = line.substr(equals + 1);
        }
        return env;
    }

    std::map<std::string, std::string> WinTc::vsEnv(const std::filesystem::path &vsRoot)
    {
        const auto vsDevCmd = vsRoot / "Common7" / "Tools" / "VsDevCmd.bat";
        const auto vcVarsAll = vsRoot / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat";
        std::string command;
        if (File::exists(vsDevCmd))
        {
            command = "call \"" + vsDevCmd.string() + "\" -arch=x64 -host_arch=x64 >nul && set";
        }
        else if (File::exists(vcVarsAll))
        {
            command = "call \"" + vcVarsAll.string() + "\" x64 >nul && set";
        }
        else
        {
            return {};
        }

#ifdef _WIN32
        auto result = Process::runShell(command, ProcessShell::Cmd);
#else
        auto result = Process::run("sh", {"-c", command});
#endif
        if (result.exitCode != 0)
        {
            return {};
        }
        auto capturedEnv = parseEnv(result.output);
        std::map<std::string, std::string> selected;
        for (const auto *key :
             {"PATH", "INCLUDE", "LIB", "LIBPATH", "WindowsSdkDir", "VCINSTALLDIR", "VCToolsInstallDir"})
        {
            auto it = capturedEnv.find(key);
            if (it != capturedEnv.end())
            {
                selected[key] = it->second;
            }
        }
        return selected;
    }

    std::filesystem::path WinTc::latestMsvc()
    {
        for (const auto &vsRoot : vsRoots())
        {
            const auto toolRoot = ToolProbe::newest(vsRoot / "VC" / "Tools" / "MSVC");
            if (!toolRoot.empty())
            {
                return toolRoot;
            }
        }
        return {};
    }

    void WinTc::addRuntime(ToolchainProfile &profile)
    {
        const auto toolRoot = latestMsvc();
        if (toolRoot.empty())
        {
            return;
        }
        ToolProbe::appendDir(profile.systemIncludeDirs, toolRoot / "include");
        ToolProbe::appendDir(profile.systemLibDirs, toolRoot / "lib" / profile.targetArch);
    }

    ToolchainProfile WinTc::clangCl(const std::filesystem::path &compiler, const std::filesystem::path &binRoot,
                                    const std::filesystem::path &toolRoot, const std::filesystem::path &sdkRoot,
                                    const WindowsTargetArch &target)
    {
        ToolchainProfile profile;
        profile.id = std::string("windows-clangcl-") + target.idSuffix;
        profile.platform = "windows";
        profile.hostArch = "x64";
        profile.targetArch = target.targetArch;
        profile.compilerKind = "clang-cl";
        profile.sdkKind = "windows-sdk";
        profile.compiler = compiler;
        profile.cxxCompiler = profile.compiler;
        profile.linker = ToolProbe::inDir(binRoot, "link");
        if (profile.linker.empty())
        {
            profile.linker = ToolchainDetector::findExecutable("lld-link");
        }
        profile.archiver = ToolProbe::inDir(binRoot, "lib");
        if (profile.archiver.empty())
        {
            profile.archiver = ToolchainDetector::findExecutable("llvm-lib");
        }
        if (!profile.compiler.empty())
        {
            ToolProbe::appendDir(profile.binaryDirs, profile.compiler.parent_path());
        }
        ToolProbe::appendDir(profile.binaryDirs, binRoot);
        ToolProbe::appendDir(profile.systemIncludeDirs, toolRoot / "include");
        ToolProbe::appendDir(profile.systemIncludeDirs, toolRoot / "atlmfc" / "include");
        ToolProbe::appendDir(profile.systemLibDirs, toolRoot / "lib" / profile.targetArch);
        ToolProbe::appendDir(profile.systemLibDirs, toolRoot / "atlmfc" / "lib" / profile.targetArch);
        addSdkRoot(profile, sdkRoot);
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        env(profile);
        ToolProbe::complete(profile);
        return profile;
    }

    void WinTc::addClangCl(std::vector<ToolchainProfile> &profiles)
    {
        const auto clangClExe = ToolchainDetector::findExecutable("clang-cl");
        if (clangClExe.empty())
        {
            return;
        }
        const auto windowsSdk = WinTc::sdkRoot();
        for (const auto &vsRoot : vsRoots())
        {
            const auto toolRoot = ToolProbe::newest(vsRoot / "VC" / "Tools" / "MSVC");
            if (toolRoot.empty())
            {
                continue;
            }
            for (const auto &target : kWindowsTargetArchs)
            {
                const auto binRoot = toolRoot / "bin" / "Hostx64" / target.binArch;
                if (!File::exists(binRoot / "link.exe") && !File::exists(binRoot / "lib.exe"))
                {
                    continue;
                }
                profiles.push_back(WinTc::clangCl(clangClExe, binRoot, toolRoot, windowsSdk, target));
            }
            return;
        }
    }

    ToolchainProfile WinTc::gcc()
    {
        ToolchainProfile profile;
        profile.id = "windows-gcc-x64";
        profile.platform = "windows";
        profile.hostArch = "x64";
        profile.targetArch = "x64";
        profile.compilerKind = "gcc";
        profile.sdkKind = "mingw";
        profile.compiler = ToolchainDetector::findExecutable("gcc");
        profile.cxxCompiler = ToolchainDetector::findExecutable("g++");
        profile.linker = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        profile.archiver = ToolchainDetector::findExecutable("ar");
        if (!profile.compiler.empty())
        {
            const auto binRoot = profile.compiler.parent_path();
            const auto mingwRoot = binRoot.parent_path();
            ToolProbe::appendDir(profile.binaryDirs, binRoot);
            ToolProbe::appendDir(profile.systemIncludeDirs, mingwRoot / "include");
            ToolProbe::appendDir(profile.systemIncludeDirs, mingwRoot / "x86_64-w64-mingw32" / "include");
            ToolProbe::appendDir(profile.systemLibDirs, mingwRoot / "lib");
            ToolProbe::appendDir(profile.systemLibDirs, mingwRoot / "x86_64-w64-mingw32" / "lib");
            profile.sdkRoot = mingwRoot;
            profile.sdkVersion = mingwRoot.filename().string();
        }
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        ToolProbe::complete(profile);
        return profile;
    }

    ToolchainProfile WinTc::clangGnu()
    {
        ToolchainProfile profile;
        profile.id = "windows-clang-x64";
        profile.platform = "windows";
        profile.hostArch = "x64";
        profile.targetArch = "x64";
        profile.compilerKind = "clang";
        profile.sdkKind = "none";
        profile.compiler = ToolchainDetector::findExecutable("clang");
        profile.cxxCompiler = ToolchainDetector::findExecutable("clang++");
        profile.linker = profile.cxxCompiler.empty() ? profile.compiler : profile.cxxCompiler;
        profile.archiver = ToolchainDetector::findExecutable("llvm-ar");
        if (profile.archiver.empty())
        {
            profile.archiver = ToolchainDetector::findExecutable("ar");
        }
        if (!profile.compiler.empty())
        {
            ToolProbe::appendDir(profile.binaryDirs, profile.compiler.parent_path());
        }
        const auto gccProfile = WinTc::gcc();
        if (!gccProfile.compiler.empty())
        {
            profile.id = "windows-clang-mingw-x64";
            profile.sdkKind = "mingw";
            profile.sdkRoot = gccProfile.sdkRoot;
            profile.sdkVersion = gccProfile.sdkVersion;
            profile.systemIncludeDirs = gccProfile.systemIncludeDirs;
            profile.systemLibDirs = gccProfile.systemLibDirs;
        }
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        ToolProbe::complete(profile);
        return profile;
    }

    ToolchainProfile WinTc::msvc(const std::filesystem::path &vsRoot, const std::filesystem::path &toolRoot,
                                 const std::filesystem::path &sdkRoot, const std::map<std::string, std::string> &vsEnv,
                                 const WindowsTargetArch &target)
    {
        const auto binRoot = toolRoot / "bin" / "Hostx64" / target.binArch;
        ToolchainProfile profile;
        profile.id = std::string("windows-msvc-") + target.idSuffix;
        profile.platform = "windows";
        profile.hostArch = "x64";
        profile.targetArch = target.targetArch;
        profile.compilerKind = "msvc";
        profile.sdkKind = "windows-sdk";
        profile.compiler = binRoot / "cl.exe";
        profile.cxxCompiler = profile.compiler;
        profile.linker = binRoot / "link.exe";
        profile.archiver = binRoot / "lib.exe";
        profile.compilerVersion = toolRoot.filename().string();
        ToolProbe::appendDir(profile.binaryDirs, binRoot);
        ToolProbe::appendDir(profile.systemIncludeDirs, toolRoot / "include");
        ToolProbe::appendDir(profile.systemIncludeDirs, toolRoot / "atlmfc" / "include");
        ToolProbe::appendDir(profile.systemLibDirs, toolRoot / "lib" / profile.targetArch);
        ToolProbe::appendDir(profile.systemLibDirs, toolRoot / "atlmfc" / "lib" / profile.targetArch);
        addSdkRoot(profile, sdkRoot);
        ToolProbe::addTools(profile);
        ToolProbe::addPath(profile);
        for (auto &[key, value] : vsEnv)
        {
            profile.environment[key] = value;
        }
        env(profile);
        ToolProbe::complete(profile);
        (void)vsRoot;
        return profile;
    }

    void WinTc::addMsvcRoots(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &vsRoot,
                             const std::filesystem::path &windowsSdkRoot, DiagnosticSink &diagnostics)
    {
        (void)diagnostics;
        const auto toolRoot = ToolProbe::newest(vsRoot / "VC" / "Tools" / "MSVC");
        if (toolRoot.empty())
        {
            return;
        }
        const auto capturedEnv = WinTc::vsEnv(vsRoot);
        const auto sdk = windowsSdkRoot.empty() ? WinTc::sdkRoot(capturedEnv) : windowsSdkRoot;
        for (const auto &target : kWindowsTargetArchs)
        {
            const auto binRoot = toolRoot / "bin" / "Hostx64" / target.binArch;
            if (!File::exists(binRoot / "cl.exe"))
            {
                continue;
            }
            profiles.push_back(msvc(vsRoot, toolRoot, sdk, capturedEnv, target));
        }
    }

    void WinTc::addMsvc(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        for (const auto &vsRoot : vsRoots())
        {
            addMsvcRoots(profiles, vsRoot, WinTc::sdkRoot(), diagnostics);
        }
    }

    void WinTc::add(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        addMsvc(profiles, diagnostics);
        addClangCl(profiles);
        auto gccProfile = WinTc::gcc();
        if (!gccProfile.compiler.empty())
        {
            profiles.push_back(std::move(gccProfile));
        }
        auto clangProfile = clangGnu();
        if (!clangProfile.compiler.empty())
        {
            profiles.push_back(std::move(clangProfile));
        }
    }

    void ToolchainDetector::windows(std::vector<ToolchainProfile> &profiles, const std::filesystem::path &vsRoot,
                                    const std::filesystem::path &windowsSdkRoot, DiagnosticSink &diagnostics)
    {
        WinTc::addMsvcRoots(profiles, vsRoot, windowsSdkRoot, diagnostics);
    }

    void ToolchainDetector::windows(std::vector<ToolchainProfile> &profiles, DiagnosticSink &diagnostics)
    {
        WinTc::add(profiles, diagnostics);
    }

} // namespace rhodium
