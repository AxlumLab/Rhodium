#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/toolchain/ToolchainDetector.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace
{
    std::filesystem::path makeTempDir()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-toolchain-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    std::filesystem::path androidPrebuiltDir(const std::filesystem::path &ndkRoot)
    {
#ifdef _WIN32
        return ndkRoot / "toolchains" / "llvm" / "prebuilt" / "windows-x86_64";
#elif __APPLE__
        return ndkRoot / "toolchains" / "llvm" / "prebuilt" / "darwin-x86_64";
#else
        return ndkRoot / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64";
#endif
    }

    void touchFile(const std::filesystem::path &path)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << "";
    }

    bool hasMissing(const rhodium::ToolchainProfile &profile, const std::string &value)
    {
        return std::find(profile.missing.begin(), profile.missing.end(), value) != profile.missing.end();
    }

    std::string toolFileName(const std::string &baseName)
    {
#ifdef _WIN32
        return baseName + ".exe";
#else
        return baseName;
#endif
    }

    void createFakeMsvcToolchain(const std::filesystem::path &vsRoot,
                                 const std::filesystem::path &sdkRoot,
                                 const std::vector<std::string> &targetArchs)
    {
        const auto toolRoot = vsRoot / "VC" / "Tools" / "MSVC" / "14.44.35207";
        std::filesystem::create_directories(toolRoot / "include");
        touchFile(toolRoot / "include" / "vector");
        for (const auto &arch : targetArchs)
        {
            const auto bin = toolRoot / "bin" / "Hostx64" / arch;
            touchFile(bin / "cl.exe");
            touchFile(bin / "link.exe");
            touchFile(bin / "lib.exe");
            touchFile(toolRoot / "lib" / arch / "msvcrt.lib");
        }

        const auto sdkVersion = sdkRoot / "Include" / "10.0.26100.0";
        std::filesystem::create_directories(sdkVersion / "ucrt");
        std::filesystem::create_directories(sdkVersion / "um");
        std::filesystem::create_directories(sdkVersion / "shared");
        std::filesystem::create_directories(sdkVersion / "winrt");
        for (const auto &arch : targetArchs)
        {
            touchFile(sdkRoot / "Lib" / "10.0.26100.0" / "ucrt" / arch / "ucrt.lib");
            touchFile(sdkRoot / "Lib" / "10.0.26100.0" / "um" / arch / "kernel32.lib");
        }
        touchFile(sdkRoot / "bin" / "10.0.26100.0" / "x64" / "rc.exe");
        touchFile(sdkRoot / "bin" / "10.0.26100.0" / "x64" / "mt.exe");
    }

    const rhodium::ToolchainProfile *findProfile(const std::vector<rhodium::ToolchainProfile> &profiles, const std::string &id)
    {
        const auto it = std::find_if(profiles.begin(), profiles.end(), [&](const auto &profile) {
            return profile.id == id;
        });
        return it == profiles.end() ? nullptr : &*it;
    }

    bool hasPathWith(const std::vector<std::filesystem::path> &paths, const std::string &needle)
    {
        return std::any_of(paths.begin(), paths.end(), [&](const auto &path) {
            return path.generic_string().find(needle) != std::string::npos;
        });
    }

    TEST(ToolchainTests, WritesAndReadsToolchainProfiles)
    {
        const auto dir = makeTempDir();
        const auto path = dir / "host.toml";

        rhodium::ToolchainProfile profile;
        profile.id = "windows-clangcl-x64";
        profile.platform = "windows";
        profile.hostArch = "x64";
        profile.targetArch = "x64";
        profile.compilerKind = "clang-cl";
        profile.sdkKind = "windows-sdk";
        profile.compiler = "D:/Sdks/llvm/bin/clang-cl.exe";
        profile.cxxCompiler = profile.compiler;
        profile.linker = "D:/Sdks/llvm/bin/lld-link.exe";
        profile.archiver = "D:/Sdks/llvm/bin/llvm-lib.exe";
        profile.cmake = "D:/Sdks/cmake/bin/cmake.exe";
        profile.strip = "D:/Sdks/llvm/bin/llvm-strip.exe";
        profile.ranlib = "D:/Sdks/llvm/bin/llvm-ranlib.exe";
        profile.adb = "C:/Android/Sdk/platform-tools/adb.exe";
        profile.java = "C:/Applications/AndroidStudio/jbr/bin/java.exe";
        profile.javac = "C:/Applications/AndroidStudio/jbr/bin/javac.exe";
        profile.gradle = "C:/Gradle/bin/gradle.bat";
        profile.sysroot = "D:/Sdks/Android/ndk/sysroot";
        profile.androidSdkRoot = "C:/Android/Sdk";
        profile.androidNdkRoot = "C:/Android/Sdk/ndk/27.0.0";
        profile.jdkRoot = "C:/Applications/AndroidStudio/jbr";
        profile.androidApi = "23";
        profile.androidAbi = "arm64-v8a";
        profile.targetTriple = "aarch64-linux-android23";
        profile.deploymentTarget = "13.0";
        profile.systemIncludeDirs.push_back("C:/SDK/include/ucrt");
        profile.systemLibDirs.push_back("C:/SDK/lib/ucrt/x64");
        profile.binaryDirs.push_back("D:/Sdks/llvm/bin");
        profile.environment["PATH"] = "D:/Sdks/llvm/bin";
        profile.complete = false;
        profile.missing = {"windows_sdk", "rc", "mt"};

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ToolchainRegistry::write(path, {profile}, diagnostics)) << diagnostics.diagnostics().dump();

        std::vector<rhodium::ToolchainProfile> loaded;
        ASSERT_TRUE(rhodium::ToolchainRegistry::read(path, loaded, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(loaded.size(), 1u);
        EXPECT_EQ(loaded[0].id, profile.id);
        EXPECT_EQ(loaded[0].compilerKind, "clang-cl");
        EXPECT_EQ(loaded[0].linker.generic_string(), "D:/Sdks/llvm/bin/lld-link.exe");
        EXPECT_EQ(loaded[0].archiver.generic_string(), "D:/Sdks/llvm/bin/llvm-lib.exe");
        EXPECT_EQ(loaded[0].strip.generic_string(), "D:/Sdks/llvm/bin/llvm-strip.exe");
        EXPECT_EQ(loaded[0].ranlib.generic_string(), "D:/Sdks/llvm/bin/llvm-ranlib.exe");
        EXPECT_EQ(loaded[0].adb.generic_string(), "C:/Android/Sdk/platform-tools/adb.exe");
        EXPECT_EQ(loaded[0].java.generic_string(), "C:/Applications/AndroidStudio/jbr/bin/java.exe");
        EXPECT_EQ(loaded[0].javac.generic_string(), "C:/Applications/AndroidStudio/jbr/bin/javac.exe");
        EXPECT_EQ(loaded[0].gradle.generic_string(), "C:/Gradle/bin/gradle.bat");
        EXPECT_EQ(loaded[0].sysroot.generic_string(), "D:/Sdks/Android/ndk/sysroot");
        EXPECT_EQ(loaded[0].androidSdkRoot.generic_string(), "C:/Android/Sdk");
        EXPECT_EQ(loaded[0].androidNdkRoot.generic_string(), "C:/Android/Sdk/ndk/27.0.0");
        EXPECT_EQ(loaded[0].jdkRoot.generic_string(), "C:/Applications/AndroidStudio/jbr");
        EXPECT_EQ(loaded[0].androidApi, "23");
        EXPECT_EQ(loaded[0].androidAbi, "arm64-v8a");
        EXPECT_EQ(loaded[0].targetTriple, "aarch64-linux-android23");
        EXPECT_EQ(loaded[0].deploymentTarget, "13.0");
        EXPECT_EQ(loaded[0].systemIncludeDirs[0].generic_string(), "C:/SDK/include/ucrt");
        EXPECT_EQ(loaded[0].environment["PATH"], "D:/Sdks/llvm/bin");
        EXPECT_FALSE(loaded[0].complete);
        EXPECT_EQ(loaded[0].missing, profile.missing);

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, ManualToolchainDoesNotRequireGit)
    {
        const auto dir = makeTempDir();
        const auto bin = dir / "bin";
        const auto include = dir / "include";
        const auto lib = dir / "lib";
        std::filesystem::create_directories(include);
        std::filesystem::create_directories(lib);
        touchFile(bin / toolFileName("clang"));
        touchFile(bin / toolFileName("clang++"));
        touchFile(bin / toolFileName("lld"));
        touchFile(bin / toolFileName("llvm-ar"));
        touchFile(bin / toolFileName("ninja"));

        rhodium::ToolchainProfile profile;
        profile.id = "manual-clang-x64";
        profile.platform = "linux";
        profile.compilerKind = "clang";
        profile.targetArch = "x64";
        profile.compiler = bin / toolFileName("clang");
        profile.cxxCompiler = bin / toolFileName("clang++");
        profile.linker = bin / toolFileName("lld");
        profile.archiver = bin / toolFileName("llvm-ar");
        profile.ninja = bin / toolFileName("ninja");
        profile.systemIncludeDirs.push_back(include);
        profile.systemLibDirs.push_back(lib);

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ToolchainRegistry::writeUser(dir, rhodium::ToolchainSource::ProjectUser, {profile}, diagnostics)) << diagnostics.diagnostics().dump();
        const auto loaded = rhodium::ToolchainRegistry::load(dir, diagnostics);
        const auto manual = std::find_if(loaded.begin(), loaded.end(), [](const auto &loadedProfile) {
            return loadedProfile.profile.id == "manual-clang-x64";
        });
        ASSERT_NE(manual, loaded.end());
        EXPECT_TRUE(manual->profile.complete);
        EXPECT_TRUE(hasMissing(manual->profile, "optional:git"));

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, DetectsMsvcX64AndX86ProfilesSideBySide)
    {
        const auto dir = makeTempDir();
        const auto vsRoot = dir / "VS";
        const auto sdkRoot = dir / "Windows Kits" / "10";
        createFakeMsvcToolchain(vsRoot, sdkRoot, {"x64", "x86"});

        rhodium::JsonDiagnosticSink diagnostics;
        std::vector<rhodium::ToolchainProfile> profiles;
        rhodium::ToolchainDetector::windows(profiles, vsRoot, sdkRoot, diagnostics);

        const auto *x64 = findProfile(profiles, "windows-msvc-x64");
        const auto *x86 = findProfile(profiles, "windows-msvc-x86");
        ASSERT_NE(x64, nullptr);
        ASSERT_NE(x86, nullptr);
        EXPECT_EQ(x64->hostArch, "x64");
        EXPECT_EQ(x64->targetArch, "x64");
        EXPECT_EQ(x86->hostArch, "x64");
        EXPECT_EQ(x86->targetArch, "x86");
        EXPECT_TRUE(hasPathWith(x86->systemLibDirs, "/lib/x86"));
        EXPECT_TRUE(hasPathWith(x86->systemLibDirs, "/ucrt/x86"));
        EXPECT_TRUE(hasPathWith(x86->systemLibDirs, "/um/x86"));
        EXPECT_FALSE(hasPathWith(x86->systemLibDirs, "/lib/x64"));
        EXPECT_FALSE(hasPathWith(x86->systemLibDirs, "/ucrt/x64"));
        EXPECT_FALSE(hasPathWith(x86->systemLibDirs, "/um/x64"));

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, MsvcX86IncompleteWhenSdkX86LibMissing)
    {
        const auto dir = makeTempDir();
        const auto vsRoot = dir / "VS";
        const auto sdkRoot = dir / "Windows Kits" / "10";
        createFakeMsvcToolchain(vsRoot, sdkRoot, {"x86"});
        std::filesystem::remove_all(sdkRoot / "Lib" / "10.0.26100.0" / "ucrt" / "x86");

        rhodium::JsonDiagnosticSink diagnostics;
        std::vector<rhodium::ToolchainProfile> profiles;
        rhodium::ToolchainDetector::windows(profiles, vsRoot, sdkRoot, diagnostics);

        const auto *x86 = findProfile(profiles, "windows-msvc-x86");
        ASSERT_NE(x86, nullptr);
        EXPECT_FALSE(x86->complete);
        EXPECT_TRUE(hasMissing(*x86, "windows_sdk_ucrt_lib_x86"));

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, FindsExecutableOnPath)
    {
        const auto cmake = rhodium::ToolchainDetector::findExecutable("cmake");
        EXPECT_TRUE(cmake.empty() || rhodium::File::exists(cmake));
    }

    TEST(ToolchainTests, WritesAndReadsLocalToolchainConfig)
    {
        const auto dir = makeTempDir();

        rhodium::LocalToolchainConfig config;
        config.preferredToolchain = "windows-msvc-x64";

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ToolchainRegistry::writeConfig(dir, config, diagnostics)) << diagnostics.diagnostics().dump();

        rhodium::LocalToolchainConfig loaded;
        ASSERT_TRUE(rhodium::ToolchainRegistry::readConfig(dir, loaded, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(loaded.preferredToolchain, "windows-msvc-x64");

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, ProcessReceivesEnvironmentOverride)
    {
        const std::map<std::string, std::string> env{{"RHODIUM_PROCESS_ENV_TEST", "rhodium-env-ok"}};
#ifdef _WIN32
        const auto result = rhodium::Process::run("cmd.exe", {"/D", "/S", "/C", "echo %RHODIUM_PROCESS_ENV_TEST%"}, env);
#else
        const auto result = rhodium::Process::run("sh", {"-c", "printf '%s' \"$RHODIUM_PROCESS_ENV_TEST\""}, env);
#endif
        ASSERT_EQ(result.exitCode, 0) << result.output;
        EXPECT_NE(result.output.find("rhodium-env-ok"), std::string::npos);
    }

    TEST(ToolchainTests, AndroidProfilesAreEmittedWhenSdkExistsButNdkMissing)
    {
        const auto dir = makeTempDir();
        const auto sdk = dir / "sdk";
        std::filesystem::create_directories(sdk / "platforms" / "android-35");
        touchFile(sdk / "platform-tools" / "adb.exe");

        rhodium::JsonDiagnosticSink diagnostics;
        std::vector<rhodium::ToolchainProfile> profiles;
        rhodium::ToolchainDetector::android(profiles, sdk, {}, diagnostics);

        ASSERT_EQ(profiles.size(), 4u);
        const auto arm64 = std::find_if(profiles.begin(), profiles.end(), [](const auto &profile) {
            return profile.id == "android-clang-arm64";
        });
        ASSERT_NE(arm64, profiles.end());
        EXPECT_FALSE(arm64->complete);
        EXPECT_EQ(arm64->androidSdkRoot, sdk);
        EXPECT_EQ(arm64->androidAbi, "arm64-v8a");
        EXPECT_TRUE(hasMissing(*arm64, "android_ndk"));
        EXPECT_TRUE(hasMissing(*arm64, "android_prebuilt_bin"));
        EXPECT_TRUE(hasMissing(*arm64, "android_clangxx"));

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, AndroidToolLookupSupportsWindowsCmdWrappers)
    {
        const auto dir = makeTempDir();
        const auto sdk = dir / "sdk";
        const auto ndk = sdk / "ndk" / "27.0.0";
        const auto bin = androidPrebuiltDir(ndk) / "bin";
        std::filesystem::create_directories(sdk / "platforms" / "android-23");
        touchFile(bin /
#ifdef _WIN32
                  "aarch64-linux-android23-clang.cmd"
#else
                  "aarch64-linux-android23-clang"
#endif
        );
        touchFile(bin /
#ifdef _WIN32
                  "aarch64-linux-android23-clang++.cmd"
#else
                  "aarch64-linux-android23-clang++"
#endif
        );
        touchFile(bin /
#ifdef _WIN32
                  "llvm-ar.exe"
#else
                  "llvm-ar"
#endif
        );

        rhodium::JsonDiagnosticSink diagnostics;
        std::vector<rhodium::ToolchainProfile> profiles;
        rhodium::ToolchainDetector::android(profiles, sdk, ndk, diagnostics);

        const auto arm64 = std::find_if(profiles.begin(), profiles.end(), [](const auto &profile) {
            return profile.id == "android-clang-arm64";
        });
        ASSERT_NE(arm64, profiles.end());
        EXPECT_NE(arm64->compiler.filename().string().find("aarch64-linux-android23-clang"), std::string::npos);
        EXPECT_NE(arm64->cxxCompiler.filename().string().find("aarch64-linux-android23-clang++"), std::string::npos);

        std::filesystem::remove_all(dir);
    }

    TEST(ToolchainTests, AndroidCompilerFallsBackToPlainClang)
    {
        const auto dir = makeTempDir();
        const auto sdk = dir / "sdk";
        const auto ndk = sdk / "ndk" / "27.0.0";
        const auto bin = androidPrebuiltDir(ndk) / "bin";
        std::filesystem::create_directories(sdk / "platforms" / "android-23");
        touchFile(bin /
#ifdef _WIN32
                  "clang.exe"
#else
                  "clang"
#endif
        );
        touchFile(bin /
#ifdef _WIN32
                  "clang++.exe"
#else
                  "clang++"
#endif
        );
        touchFile(bin /
#ifdef _WIN32
                  "llvm-ar.exe"
#else
                  "llvm-ar"
#endif
        );

        rhodium::JsonDiagnosticSink diagnostics;
        std::vector<rhodium::ToolchainProfile> profiles;
        rhodium::ToolchainDetector::android(profiles, sdk, ndk, diagnostics);

        const auto arm64 = std::find_if(profiles.begin(), profiles.end(), [](const auto &profile) {
            return profile.id == "android-clang-arm64";
        });
        ASSERT_NE(arm64, profiles.end());
        EXPECT_EQ(arm64->compiler.filename().string(), toolFileName("clang"));
        EXPECT_EQ(arm64->cxxCompiler.filename().string(), toolFileName("clang++"));
        EXPECT_EQ(arm64->targetTriple, "aarch64-linux-android23");

        std::filesystem::remove_all(dir);
    }
}
