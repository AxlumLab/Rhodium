#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/package/PackageCache.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <chrono>

namespace rhodium::tests::lockFileTests
{

    TEST(LockFileTests, WritesAndReadsPackages)
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto dir = std::filesystem::temp_directory_path() / ("rhodium-lock-" + std::to_string(stamp));
        std::filesystem::create_directories(dir);
        const auto path = dir / "rhodium.lock.toml";

        rhodium::LockFile lock;
        rhodium::LockedPackage package;
        package.name = "fmt";
        package.source = "github.com/fmtlib/fmt";
        package.sourceType = "git";
        package.ref = "10.2.1";
        package.commit = "abcdef";
        package.subdir = ".";
        package.buildType = "cmake";
        package.config = "release";
        package.toolchainId = "windows-msvc-x64";
        package.buildHash = "hash";
        package.artifactId = "fmt/abcdef";
        package.buildOptions["FMT_TEST"] = "OFF";
        package.artifacts.includeDirs.push_back("include");
        package.artifacts.libs.push_back("fmt");
        lock.packages.push_back(package);

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::LockFile::write(path, lock, diagnostics)) << diagnostics.diagnostics().dump();

        rhodium::LockFile loaded;
        ASSERT_TRUE(rhodium::LockFile::read(path, loaded, diagnostics)) << diagnostics.diagnostics().dump();

        ASSERT_EQ(loaded.packages.size(), 1u);
        EXPECT_EQ(loaded.packages[0].name, "fmt");
        EXPECT_EQ(loaded.packages[0].source, "github.com/fmtlib/fmt");
        EXPECT_EQ(loaded.packages[0].commit, "abcdef");
        EXPECT_EQ(loaded.packages[0].buildHash, "hash");
        EXPECT_EQ(loaded.packages[0].config, "release");
        EXPECT_EQ(loaded.packages[0].toolchainId, "windows-msvc-x64");
        EXPECT_EQ(loaded.packages[0].artifactId, "fmt/abcdef");
        EXPECT_EQ(loaded.packages[0].buildOptions["FMT_TEST"], "OFF");
        EXPECT_EQ(loaded.packages[0].artifacts.includeDirs[0], "include");
        EXPECT_EQ(loaded.packages[0].artifacts.libs[0], "fmt");
        EXPECT_EQ(rhodium::File::read(path).find("E:"), std::string::npos);

        std::filesystem::remove_all(dir);
    }

    TEST(LockFileTests, PackageCacheUsesShortSourceHashAndRefForRemoteDependencies)
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "fmt";
        dependency.source = "github.com/fmtlib/fmt";
        dependency.ref = "10.2.1";

        rhodium::PackageCache cache(".tpkg/packages");
        const auto path = cache.source(dependency).generic_string();

        EXPECT_NE(path.find(".tpkg/packages/fmt/"), std::string::npos);
        EXPECT_NE(path.find("/10.2.1"), std::string::npos);
        EXPECT_EQ(path.find("github.com/fmtlib/fmt"), std::string::npos);
    }

    TEST(LockFileTests, ArtifactIdIncludesBuildHash)
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "fmt";
        dependency.source = "github.com/fmtlib/fmt";
        dependency.ref = "10.2.1";
        dependency.buildType = "cmake";
        dependency.cmake.options["FMT_TEST"] = "OFF";

        const auto first = rhodium::LockedPackage::from(dependency, "abcdef", "debug", "windows-msvc-x64");
        dependency.cmake.options["FMT_TEST"] = "ON";
        const auto second = rhodium::LockedPackage::from(dependency, "abcdef", "debug", "windows-msvc-x64");

        EXPECT_NE(first.buildHash, second.buildHash);
        EXPECT_NE(first.artifactId, second.artifactId);
        EXPECT_EQ(first.artifactId, "fmt/abcdef-" + first.buildHash);
    }

    TEST(LockFileTests, WritesAndReadsConfigToolchainFields)
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto dir = std::filesystem::temp_directory_path() / ("rhodium-lock-matrix-" + std::to_string(stamp));
        std::filesystem::create_directories(dir);

        rhodium::LockFile lock;
        rhodium::LockedPackage package;
        package.name = "fmt";
        package.source = "github.com/fmtlib/fmt";
        package.sourceType = "git";
        package.config = "debug";
        package.toolchainId = "android-clang-arm64";
        package.artifactId = "fmt/hash";
        lock.packages.push_back(package);

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::LockFile::write(dir / "rhodium.lock.toml", lock, diagnostics))
            << diagnostics.diagnostics().dump();

        rhodium::LockFile loaded;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "rhodium.lock.toml", loaded, diagnostics))
            << diagnostics.diagnostics().dump();
        ASSERT_EQ(loaded.packages.size(), 1u);
        EXPECT_EQ(loaded.packages[0].config, "debug");
        EXPECT_EQ(loaded.packages[0].toolchainId, "android-clang-arm64");

        std::filesystem::remove_all(dir);
    }

    TEST(LockFileTests, SamePackageDebugReleaseCanCoexist)
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "fmt";
        dependency.source = "github.com/fmtlib/fmt";
        dependency.ref = "10.2.1";

        const auto debug = rhodium::LockedPackage::from(dependency, "abcdef", "debug", "windows-msvc-x64");
        const auto release = rhodium::LockedPackage::from(dependency, "abcdef", "release", "windows-msvc-x64");

        EXPECT_EQ(debug.name, release.name);
        EXPECT_EQ(debug.toolchainId, release.toolchainId);
        EXPECT_NE(debug.config, release.config);
        EXPECT_NE(debug.buildHash, release.buildHash);
        EXPECT_NE(debug.artifactId, release.artifactId);
    }

    TEST(LockFileTests, SamePackageDifferentToolchainsCanCoexist)
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "fmt";
        dependency.source = "github.com/fmtlib/fmt";
        dependency.ref = "10.2.1";

        const auto windows = rhodium::LockedPackage::from(dependency, "abcdef", "debug", "windows-msvc-x64");
        const auto android = rhodium::LockedPackage::from(dependency, "abcdef", "debug", "android-clang-arm64");

        EXPECT_EQ(windows.name, android.name);
        EXPECT_EQ(windows.config, android.config);
        EXPECT_NE(windows.toolchainId, android.toolchainId);
        EXPECT_NE(windows.buildHash, android.buildHash);
        EXPECT_NE(windows.artifactId, android.artifactId);
    }

    TEST(LockFileTests, ToolchainContentHashChangesArtifactId)
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "fmt";
        dependency.source = "github.com/fmtlib/fmt";
        dependency.sourceType = "git";
        dependency.ref = "10.2.1";
        dependency.buildType = "cmake";

        const auto first =
            rhodium::LockedPackage::from(dependency, "abcdef", "debug", "windows-msvc-x64", "toolchain-content-a");
        const auto second =
            rhodium::LockedPackage::from(dependency, "abcdef", "debug", "windows-msvc-x64", "toolchain-content-b");

        EXPECT_NE(first.buildHash, second.buildHash);
        EXPECT_NE(first.artifactId, second.artifactId);
    }

    TEST(LockFileTests, OverrideAndNormalCanCoexist)
    {
        rhodium::DependencyDesc normal;
        normal.name = "fmt";
        normal.source = "github.com/fmtlib/fmt";
        normal.ref = "10.2.1";

        auto override = normal;
        override.overridden = true;
        override.overridePath = "C:/dev/fmt";
        override.source = override.overridePath;
        override.sourceType = "local";
        override.ref.clear();

        const auto normalLock = rhodium::LockedPackage::from(normal, "abcdef", "debug", "windows-msvc-x64");
        const auto overrideLock = rhodium::LockedPackage::from(override, "override", "debug", "windows-msvc-x64");

        EXPECT_FALSE(normalLock.overridden);
        EXPECT_TRUE(overrideLock.overridden);
        EXPECT_NE(normalLock.artifactId, overrideLock.artifactId);
    }

    TEST(LockFileTests, WritesAndReadsArchiveDependencyLockEntry)
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto dir = std::filesystem::temp_directory_path() / ("rhodium-lock-archive-" + std::to_string(stamp));
        std::filesystem::create_directories(dir);
        const auto path = dir / "rhodium.lock.toml";

        rhodium::LockFile lock;
        rhodium::LockedPackage package;
        package.name = "hello";
        package.source = "third_party/hello.zip";
        package.sourceType = "archive";
        package.sha256 = "archive-sha";
        package.resolvedArchiveHash = "archive-sha";
        package.commit = "archive-sha";
        package.stripComponents = 1;
        package.patches = {"patches/hello.patch"};
        package.patchHashes = {"patch-sha"};
        package.buildHash = "build-hash";
        package.artifactId = "hello/archive-sha-build-hash";
        lock.packages.push_back(package);

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::LockFile::write(path, lock, diagnostics)) << diagnostics.diagnostics().dump();

        rhodium::LockFile loaded;
        ASSERT_TRUE(rhodium::LockFile::read(path, loaded, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(loaded.packages.size(), 1u);
        EXPECT_EQ(loaded.packages[0].sourceType, "archive");
        EXPECT_EQ(loaded.packages[0].sha256, "archive-sha");
        EXPECT_EQ(loaded.packages[0].resolvedArchiveHash, "archive-sha");
        EXPECT_EQ(loaded.packages[0].stripComponents, 1);
        EXPECT_EQ(loaded.packages[0].patches[0], "patches/hello.patch");
        EXPECT_EQ(loaded.packages[0].patchHashes[0], "patch-sha");

        std::filesystem::remove_all(dir);
    }

    TEST(LockFileTests, ArchiveHashAndPatchHashAffectPkgHash)
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "hello";
        dependency.source = "third_party/hello.zip";
        dependency.sourceType = "archive";
        dependency.resolvedArchiveHash = "archive-a";
        dependency.patchHashes = {"patch-a"};

        const auto first = rhodium::LockedPackage::from(dependency, "archive-a", "debug", "windows-msvc-x64");
        dependency.patchHashes = {"patch-b"};
        const auto second = rhodium::LockedPackage::from(dependency, "archive-a", "debug", "windows-msvc-x64");
        dependency.resolvedArchiveHash = "archive-b";
        const auto third = rhodium::LockedPackage::from(dependency, "archive-b", "debug", "windows-msvc-x64");

        EXPECT_NE(first.buildHash, second.buildHash);
        EXPECT_NE(second.buildHash, third.buildHash);
        EXPECT_NE(first.artifactId, second.artifactId);
    }

} // namespace rhodium::tests::lockFileTests
