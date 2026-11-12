#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Hash.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/package/ArtifactGc.hpp"
#include "rhodium/package/PackageCache.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

namespace
{
    std::filesystem::path makeGcTempDir()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-artifact-gc-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    void makeArtifactDir(const std::filesystem::path &root, const std::string &artifactId)
    {
        std::filesystem::create_directories(root / ".tpkg" / "artifacts" / artifactId);
    }

    void makeBuildCacheDir(const std::filesystem::path &root, const std::string &package, const std::string &cache)
    {
        std::filesystem::create_directories(root / ".tpkg" / "build-packages" / package / cache);
    }

    rhodium::LockedPackage lockedPackage(const std::string &name)
    {
        rhodium::LockedPackage package;
        package.name = name;
        package.artifactId = name + "/live";
        package.sourceType = "git";
        package.source = "https://example.invalid/" + name + ".git";
        package.ref = "v1";
        package.config = "debug";
        package.toolchainId = "windows-msvc-x64";
        return package;
    }

    TEST(ArtifactGcTests, DryRunDoesNotDeleteUnreferencedArtifact)
    {
        const auto root = makeGcTempDir();
        makeArtifactDir(root, "fmt/live");
        makeArtifactDir(root, "fmt/stale");

        rhodium::LockFile lockFile;
        rhodium::LockedPackage package;
        package.name = "fmt";
        package.artifactId = "fmt/live";
        lockFile.packages.push_back(package);

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactGc::runArtifacts(lockFile, root, false, {}, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(rhodium::File::dir(root / ".tpkg" / "artifacts" / "fmt" / "stale"));

        std::filesystem::remove_all(root);
    }

    TEST(ArtifactGcTests, ApplyDeletesUnreferencedArtifact)
    {
        const auto root = makeGcTempDir();
        makeArtifactDir(root, "fmt/live");
        makeArtifactDir(root, "fmt/stale");

        rhodium::LockFile lockFile;
        rhodium::LockedPackage package;
        package.name = "fmt";
        package.artifactId = "fmt/live";
        lockFile.packages.push_back(package);

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactGc::runArtifacts(lockFile, root, true, "fmt", diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(rhodium::File::dir(root / ".tpkg" / "artifacts" / "fmt" / "live"));
        EXPECT_FALSE(rhodium::File::dir(root / ".tpkg" / "artifacts" / "fmt" / "stale"));

        std::filesystem::remove_all(root);
    }

    TEST(ArtifactGcTests, ApplyDeletesStaleBuildCaches)
    {
        const auto root = makeGcTempDir();
        makeBuildCacheDir(root, "fmt", "debug-windows-msvc-x64");
        makeBuildCacheDir(root, "fmt", "release-windows-msvc-x64");
        makeBuildCacheDir(root, "old", "debug-windows-msvc-x64");

        rhodium::LockFile lockFile;
        lockFile.packages.push_back(lockedPackage("fmt"));

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactGc::runBuildCaches(lockFile, root, rhodium::BuildCacheGcOptions{"debug", "windows-msvc-x64", true, {}}, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(rhodium::File::dir(root / ".tpkg" / "build-packages" / "fmt" / "debug-windows-msvc-x64"));
        EXPECT_FALSE(rhodium::File::dir(root / ".tpkg" / "build-packages" / "fmt" / "release-windows-msvc-x64"));
        EXPECT_FALSE(rhodium::File::dir(root / ".tpkg" / "build-packages" / "old" / "debug-windows-msvc-x64"));

        std::filesystem::remove_all(root);
    }

    TEST(ArtifactGcTests, DryRunDoesNotDeleteStaleBuildCaches)
    {
        const auto root = makeGcTempDir();
        makeBuildCacheDir(root, "fmt", "debug-windows-msvc-x64");
        makeBuildCacheDir(root, "fmt", "release-windows-msvc-x64");

        rhodium::LockFile lockFile;
        lockFile.packages.push_back(lockedPackage("fmt"));

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactGc::runBuildCaches(lockFile, root, rhodium::BuildCacheGcOptions{"debug", "windows-msvc-x64", false, {}}, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(rhodium::File::dir(root / ".tpkg" / "build-packages" / "fmt" / "release-windows-msvc-x64"));

        std::filesystem::remove_all(root);
    }

    TEST(ArtifactGcTests, ApplyDeletesStaleGitSourceCachesForPackage)
    {
        const auto root = makeGcTempDir();
        rhodium::LockFile lockFile;
        auto package = lockedPackage("fmt");
        package.ref = "v2";
        lockFile.packages.push_back(package);

        rhodium::DependencyDesc dependency;
        dependency.name = package.name;
        dependency.source = package.source;
        dependency.ref = package.ref;
        rhodium::PackageCache cache(root / ".tpkg" / "packages");
        std::filesystem::create_directories(cache.source(dependency) / ".git");
        std::filesystem::create_directories(cache.sourceRoot(dependency) / "v1" / ".git");

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, true, "fmt", diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(rhodium::File::dir(cache.source(dependency)));
        EXPECT_FALSE(rhodium::File::dir(cache.sourceRoot(dependency) / "v1"));

        std::filesystem::remove_all(root);
    }

    TEST(ArtifactGcTests, ApplyDeletesStaleArchiveSourceCachesForPackage)
    {
        const auto root = makeGcTempDir();
        rhodium::LockFile lockFile;
        rhodium::LockedPackage package;
        package.name = "zlib";
        package.sourceType = "archive";
        package.source = "https://example.invalid/zlib.tar.gz";
        package.resolvedArchiveHash = rhodium::Hash::xxhash64Hex("live");
        package.artifactId = "zlib/live";
        lockFile.packages.push_back(package);

        const auto archiveRoot = root / ".tpkg" / "packages" / "_archives" / "zlib";
        const auto liveCacheKey = rhodium::Hash::xxhash64Hex(package.source);
        std::filesystem::create_directories(archiveRoot / liveCacheKey / "src");
        std::filesystem::create_directories(archiveRoot / rhodium::Hash::xxhash64Hex("stale") / "src");

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, true, "zlib", diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(rhodium::File::dir(archiveRoot / liveCacheKey));
        EXPECT_FALSE(rhodium::File::dir(archiveRoot / rhodium::Hash::xxhash64Hex("stale")));

        std::filesystem::remove_all(root);
    }
}

TEST(ArtifactGcTests, KeepsNestedLiveGitRefsAndPrunesUnusedSiblings)
{
    const auto root = makeGcTempDir();
    auto package = lockedPackage("fmt");
    package.ref = "feature/foo";
    rhodium::LockFile lockFile;
    lockFile.packages.push_back(package);
    rhodium::DependencyDesc dependency;
    dependency.name = package.name;
    dependency.source = package.source;
    dependency.ref = package.ref;
    rhodium::PackageCache cache(root / ".tpkg" / "packages");
    const auto source = cache.source(dependency);
    std::filesystem::create_directories(source / ".git");
    std::filesystem::create_directories(cache.sourceRoot(dependency) / "feature/bar/.git");
    std::filesystem::create_directories(cache.sourceRoot(dependency) / "old/.git");

    rhodium::JsonDiagnosticSink diagnostics;
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, false, "fmt", diagnostics));
    EXPECT_TRUE(std::filesystem::is_directory(cache.sourceRoot(dependency) / "feature/bar"));
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, true, "fmt", diagnostics));
    EXPECT_TRUE(std::filesystem::is_directory(source / ".git"));
    EXPECT_FALSE(std::filesystem::exists(cache.sourceRoot(dependency) / "feature/bar"));
    EXPECT_FALSE(std::filesystem::exists(cache.sourceRoot(dependency) / "old"));
    std::filesystem::remove_all(root);
}

TEST(ArtifactGcTests, DoesNotTraverseSymlinkedRefParents)
{
    const auto root = makeGcTempDir();
    auto package = lockedPackage("fmt");
    package.ref = "feature/foo";
    rhodium::LockFile lockFile;
    lockFile.packages.push_back(package);
    rhodium::DependencyDesc dependency;
    dependency.name = package.name;
    dependency.source = package.source;
    dependency.ref = package.ref;
    rhodium::PackageCache cache(root / ".tpkg/packages");
    std::filesystem::create_directories(cache.sourceRoot(dependency));
    std::filesystem::create_directories(root / "outside/foo");
    std::filesystem::create_directories(root / "outside/stale");
    std::error_code ec;
    std::filesystem::create_directory_symlink(root / "outside", cache.sourceRoot(dependency) / "feature", ec);
    if (ec)
    {
        std::filesystem::remove_all(root);
        GTEST_SKIP() << "directory symlinks are unavailable: " << ec.message();
    }
    rhodium::JsonDiagnosticSink diagnostics;
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, true, "fmt", diagnostics));
    EXPECT_TRUE(std::filesystem::is_directory(root / "outside/stale"));
    std::filesystem::remove_all(root);
}

TEST(ArtifactGcTests, ReclaimsOldUrlsAndRemovedGitPackagesFromTheDiskInventory)
{
    const auto root = makeGcTempDir();
    auto package = lockedPackage("fmt");
    package.ref = "feature/current";
    rhodium::LockFile lockFile;
    lockFile.packages.push_back(package);
    rhodium::DependencyDesc dependency;
    dependency.name = package.name;
    dependency.source = package.source;
    dependency.ref = package.ref;
    rhodium::PackageCache cache(root / ".tpkg/packages");
    const auto live = cache.source(dependency);
    std::filesystem::create_directories(live / ".git");
    dependency.source = "https://example.invalid/old-url.git";
    const auto oldUrl = cache.sourceRoot(dependency);
    std::filesystem::create_directories(cache.source(dependency) / ".git");
    const auto orphan = cache.root() / "removed/url/ref";
    std::filesystem::create_directories(orphan / ".git");

    rhodium::JsonDiagnosticSink diagnostics;
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, false, {}, diagnostics));
    EXPECT_TRUE(std::filesystem::exists(oldUrl));
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, true, "fmt", diagnostics));
    EXPECT_TRUE(std::filesystem::is_directory(live / ".git"));
    EXPECT_FALSE(std::filesystem::exists(oldUrl));
    EXPECT_TRUE(std::filesystem::exists(orphan));
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lockFile, root, true, {}, diagnostics));
    EXPECT_FALSE(std::filesystem::exists(orphan));
    EXPECT_TRUE(std::filesystem::is_directory(live / ".git"));
    std::filesystem::remove_all(root);
}

TEST(ArtifactGcTests, PreservesCacheDirectoriesExplicitlyUsedAsLocalDependencySources)
{
    const auto root = makeGcTempDir();
    const auto artifact = root / ".tpkg/artifacts/cache/live";
    const auto build = root / ".tpkg/build-packages/cache/live";
    const auto source = root / ".tpkg/packages/cache/url/live";
    rhodium::LockFile lock;
    int index = 0;
    for (const auto &path : {artifact, build, source})
    {
        std::filesystem::create_directories(path);
        std::filesystem::create_directories(path.parent_path() / "stale");
        rhodium::LockedPackage package;
        package.name = "LocalAlias" + std::to_string(index++);
        package.sourceType = "local";
        package.source = path.string();
        package.artifactId = package.name + "/local";
        lock.packages.push_back(package);
    }
    rhodium::JsonDiagnosticSink diagnostics;
    ASSERT_TRUE(rhodium::ArtifactGc::runArtifacts(lock, root, true, {}, diagnostics));
    ASSERT_TRUE(rhodium::ArtifactGc::runBuildCaches(lock, root, rhodium::BuildCacheGcOptions{"debug", "", true, {}}, diagnostics));
    ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lock, root, true, {}, diagnostics)) << diagnostics.diagnostics().dump();
    for (const auto &path : {artifact, build, source})
    {
        EXPECT_TRUE(std::filesystem::is_directory(path));
        EXPECT_FALSE(std::filesystem::exists(path.parent_path() / "stale"));
    }
    std::filesystem::remove_all(root);
}

TEST(ArtifactGcTests, InvalidSourceLockEntriesDoNotAuthorizeSourceDeletion)
{
    const auto root = makeGcTempDir();
    std::filesystem::create_directories(root / ".tpkg/packages/keep/url/ref");
    auto invalid = lockedPackage("fmt");
    invalid.source.clear();
    rhodium::LockFile lock;
    lock.packages.push_back(invalid);
    rhodium::JsonDiagnosticSink diagnostics;
    EXPECT_FALSE(rhodium::ArtifactGc::runSourceCaches(lock, root, true, {}, diagnostics));
    EXPECT_TRUE(std::filesystem::is_directory(root / ".tpkg/packages/keep/url/ref"));
    std::filesystem::remove_all(root);
}
