/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */
#include "rhodium/config/LockFile.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/package/PackageArtifact.hpp"
#include "rhodium/package/PackageArtifactStore.hpp"
#include "rhodium/resolve/PackageArtifactResolver.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace rhodium::tests::packageArtifactResolverTests
{

    std::filesystem::path makeTempDir()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-artifact-resolve-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    void writeArtifact(const std::filesystem::path &root, const std::string &artifactId, const std::string &lib)
    {
        rhodium::PackageArtifact artifact;
        artifact.name = "Pkg";
        artifact.commit = "local";
        artifact.artifactId = artifactId;
        artifact.root = root / ".tpkg/artifacts" / artifactId;
        artifact.artifacts.libDirs.push_back("lib");
        artifact.artifacts.libs.push_back(lib);
        std::filesystem::create_directories(artifact.root / "lib");
#ifdef _WIN32
        std::ofstream(artifact.root / "lib" / (lib + ".lib")).put('\0');
#else
        std::ofstream(artifact.root / "lib" / ("lib" + lib + ".a")).put('\0');
#endif
        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactStore::write(artifact, diagnostics)) << diagnostics.diagnostics().dump();
    }

    rhodium::LockedPackage lockedPackage(const std::string &artifactId, const std::string &config,
                                         const std::string &toolchain, bool overridden = false)
    {
        rhodium::LockedPackage locked;
        locked.name = "Pkg";
        locked.source = overridden ? "C:/override/Pkg" : "third_party/Pkg";
        locked.sourceType = overridden ? "local" : "local";
        locked.config = config;
        locked.toolchainId = toolchain;
        locked.artifactId = artifactId;
        locked.overridden = overridden;
        locked.overridePath = overridden ? "C:/override/Pkg" : "";
        return locked;
    }

    TEST(PackageArtifactResolverTests, ResolvesArtifactMetadataToPaths)
    {
        const auto root = makeTempDir();

        rhodium::PackageArtifact artifact;
        artifact.name = "Pkg";
        artifact.commit = "local";
        artifact.artifactId = "Pkg/local";
        artifact.root = root / ".tpkg/artifacts/Pkg/local";
        artifact.artifacts.includeDirs.push_back("include");
        artifact.artifacts.libDirs.push_back("lib");
        artifact.artifacts.binDirs.push_back("bin");
        artifact.artifacts.libs.push_back("Pkg");
        artifact.artifacts.defines.push_back("PKG=1");

        std::filesystem::create_directories(artifact.root / "include");
        std::filesystem::create_directories(artifact.root / "lib");
        std::filesystem::create_directories(artifact.root / "bin");
#ifdef _WIN32
        std::ofstream(artifact.root / "lib" / "Pkg.lib").put('\0');
#else
        std::ofstream(artifact.root / "lib" / "libPkg.a").put('\0');
#endif

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactStore::write(artifact, diagnostics)) << diagnostics.diagnostics().dump();

        rhodium::LockedPackage locked;
        locked.name = "Pkg";
        locked.artifactId = "Pkg/local";
        rhodium::LockFile lockFile;
        lockFile.packages.push_back(locked);

        rhodium::PackageArtifactResolver resolver;
        ASSERT_TRUE(resolver.load(lockFile, root, diagnostics)) << diagnostics.diagnostics().dump();

        const auto *resolved = resolver.find("Pkg");
        ASSERT_NE(resolved, nullptr);
        EXPECT_EQ(resolved->includeDirs[0], artifact.root / "include");
        EXPECT_EQ(resolved->libDirs[0], artifact.root / "lib");
        EXPECT_EQ(resolved->binDirs[0], artifact.root / "bin");
        EXPECT_EQ(resolved->libs[0], "Pkg");
        EXPECT_EQ(resolved->defines[0], "PKG=1");

        std::filesystem::remove_all(root);
    }

    TEST(PackageArtifactResolverTests, LoadsOnlyMatchingConfigToolchain)
    {
        const auto root = makeTempDir();
        writeArtifact(root, "Pkg/debug-win", "PkgDebug");
        writeArtifact(root, "Pkg/release-win", "PkgRelease");
        writeArtifact(root, "Pkg/debug-android", "PkgAndroid");

        rhodium::LockFile lockFile;
        lockFile.packages.push_back(lockedPackage("Pkg/release-win", "release", "windows-msvc-x64"));
        lockFile.packages.push_back(lockedPackage("Pkg/debug-android", "debug", "android-clang-arm64"));
        lockFile.packages.push_back(lockedPackage("Pkg/debug-win", "debug", "windows-msvc-x64"));

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::PackageArtifactResolver resolver;
        ASSERT_TRUE(resolver.load(lockFile, root, rhodium::BuildSelection{"debug", "windows-msvc-x64"},
                                  rhodium::DependencyOverrideSet{}, diagnostics))
            << diagnostics.diagnostics().dump();
        const auto *resolved = resolver.find("Pkg");
        ASSERT_NE(resolved, nullptr);
        ASSERT_EQ(resolved->libs.size(), 1u);
        EXPECT_EQ(resolved->libs[0], "PkgDebug");

        std::filesystem::remove_all(root);
    }

    TEST(PackageArtifactResolverTests, LoadsOverrideArtifactOnlyWhenOverrideEnabled)
    {
        const auto root = makeTempDir();
        writeArtifact(root, "Pkg/normal", "PkgNormal");
        writeArtifact(root, "Pkg/override", "PkgOverride");

        rhodium::LockFile lockFile;
        lockFile.packages.push_back(lockedPackage("Pkg/normal", "debug", "windows-msvc-x64"));
        lockFile.packages.push_back(lockedPackage("Pkg/override", "debug", "windows-msvc-x64", true));

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::PackageArtifactResolver resolver;
        ASSERT_TRUE(resolver.load(lockFile, root, rhodium::BuildSelection{"debug", "windows-msvc-x64"},
                                  rhodium::DependencyOverrideSet{}, diagnostics))
            << diagnostics.diagnostics().dump();
        ASSERT_NE(resolver.find("Pkg"), nullptr);
        EXPECT_EQ(resolver.find("Pkg")->libs[0], "PkgNormal");

        rhodium::DependencyOverrideSet overrides;
        overrides.overrides["Pkg"].path = "C:/override/Pkg";
        ASSERT_TRUE(
            resolver.load(lockFile, root, rhodium::BuildSelection{"debug", "windows-msvc-x64"}, overrides, diagnostics))
            << diagnostics.diagnostics().dump();
        ASSERT_NE(resolver.find("Pkg"), nullptr);
        EXPECT_EQ(resolver.find("Pkg")->libs[0], "PkgOverride");

        std::filesystem::remove_all(root);
    }

} // namespace rhodium::tests::packageArtifactResolverTests
