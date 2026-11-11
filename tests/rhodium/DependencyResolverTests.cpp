#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/model/BuildModel.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/resolve/DependencyResolver.hpp"
#include "rhodium/utils/Sha256.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{

    std::filesystem::path makeTempDir()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-resolve-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    void writeText(const std::filesystem::path &path, const std::string &content)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << content;
    }

    bool runRequiredProcess(const std::string &executable, const std::vector<std::string> &args, std::string &output)
    {
        auto result = rhodium::Process::run(executable, args);
        output = result.output;
        return result.exitCode == 0;
    }

    bool directoryHasFiles(const std::filesystem::path &path)
    {
        if (!std::filesystem::is_directory(path))
        {
            return false;
        }
        return std::filesystem::directory_iterator(path) != std::filesystem::directory_iterator();
    }

    std::size_t directoryCount(const std::filesystem::path &path)
    {
        if (!std::filesystem::is_directory(path))
        {
            return 0;
        }
        std::size_t count = 0;
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                ++count;
            }
        }
        return count;
    }

    rhodium::BuildModel localHeaderOnlyModel(const std::string &name = "HeaderOnly")
    {
        rhodium::BuildModel model;
        rhodium::DependencyDesc dependency;
        dependency.name = name;
        dependency.source = "third_party/" + name;
        dependency.sourceType = "local";
        dependency.local = true;
        dependency.buildType = "header_only";
        dependency.artifacts.includeDirs.push_back("include");
        model.rootPackage.dependencies.push_back(dependency);
        return model;
    }

    void createZipEntry(struct archive *zip, const std::string &name, const std::string &content)
    {
        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, name.c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, static_cast<la_int64_t>(content.size()));
        ASSERT_EQ(archive_write_header(zip, entry), ARCHIVE_OK);
        ASSERT_EQ(archive_write_data(zip, content.data(), content.size()), static_cast<la_ssize_t>(content.size()));
        archive_entry_free(entry);
    }

    void createHelloZip(const std::filesystem::path &path)
    {
        struct archive *zip = archive_write_new();
        archive_write_set_format_zip(zip);
        ASSERT_EQ(archive_write_open_filename(zip, path.string().c_str()), ARCHIVE_OK);
        createZipEntry(zip, "hello-lib-1.0/include/hello/hello.hpp", "#pragma once\n");
        archive_write_close(zip);
        archive_write_free(zip);
    }

    TEST(DependencyResolverTests, RestoresLocalHeaderOnlyAndPrebuiltPackages)
    {
        const auto dir = makeTempDir();
        std::filesystem::create_directories(dir / "third_party/HeaderOnly/include");
        std::filesystem::create_directories(dir / "third_party/Prebuilt/include");
        std::filesystem::create_directories(dir / "third_party/Prebuilt/lib");
        std::filesystem::create_directories(dir / "third_party/Prebuilt/bin");
        writeText(dir / "third_party/Prebuilt/lib/Prebuilt.lib", "");

        rhodium::BuildModel model;
        rhodium::DependencyDesc headerOnly;
        headerOnly.name = "HeaderOnly";
        headerOnly.source = "third_party/HeaderOnly";
        headerOnly.sourceType = "local";
        headerOnly.local = true;
        headerOnly.buildType = "header_only";
        headerOnly.artifacts.includeDirs.push_back("include");
        model.rootPackage.dependencies.push_back(headerOnly);

        rhodium::DependencyDesc prebuilt;
        prebuilt.name = "Prebuilt";
        prebuilt.source = "third_party/Prebuilt";
        prebuilt.sourceType = "local";
        prebuilt.local = true;
        prebuilt.buildType = "prebuilt";
        prebuilt.artifacts.includeDirs.push_back("include");
        prebuilt.artifacts.libDirs.push_back("lib");
        prebuilt.artifacts.binDirs.push_back("bin");
        prebuilt.artifacts.libs.push_back("Prebuilt");
        model.rootPackage.dependencies.push_back(prebuilt);

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics)) << diagnostics.diagnostics().dump();

        EXPECT_TRUE(rhodium::File::exists(dir / "tpkg.lock.toml"));
        rhodium::LockFile lockFile;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", lockFile, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(lockFile.packages.size(), 2u);
        for (const auto &locked : lockFile.packages)
        {
            EXPECT_FALSE(locked.buildHash.empty());
            EXPECT_EQ(locked.artifactId, locked.name + "/" + locked.commit + "-" + locked.buildHash);
            EXPECT_TRUE(rhodium::File::exists(dir / ".rhodium/artifacts" / locked.artifactId / "artifact.toml"));
        }
        EXPECT_EQ(rhodium::File::read(dir / "tpkg.lock.toml").find("E:"), std::string::npos);
        EXPECT_EQ(rhodium::File::read(dir / "tpkg.lock.toml").find("cache_path"), std::string::npos);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RestoresLocalGitCMakePackageByCommit)
    {
        const auto dir = makeTempDir();
        const auto repo = dir / "TinyCMakeLib";
        writeText(repo / "CMakeLists.txt", R"cmake(
cmake_minimum_required(VERSION 3.20)
project(TinyCMakeLib LANGUAGES CXX)
add_library(TinyCMakeLib STATIC src/Tiny.cpp)
target_include_directories(TinyCMakeLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
install(TARGETS TinyCMakeLib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
)cmake");
        writeText(repo / "include/tiny/Tiny.hpp", R"cpp(
#pragma once
namespace tiny {
int value();
}
)cpp");
        writeText(repo / "src/Tiny.cpp", R"cpp(
#include <tiny/Tiny.hpp>
namespace tiny {
int value() {
    return 42;
}
}
)cpp");

        std::string processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "init"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "checkout", "-B", "master"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "config", "user.email", "tiny@example.invalid"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "config", "user.name", "Tiny Test"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "add", "."}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "commit", "-m", "init tiny cmake lib"}, processOutput)) << processOutput;

        rhodium::BuildModel model;
        rhodium::DependencyDesc dependency;
        dependency.name = "TinyCMakeLib";
        dependency.source = "TinyCMakeLib";
        dependency.sourceType = "git";
        dependency.ref = "master";
        dependency.subdir = ".";
        dependency.buildType = "cmake";
        dependency.artifacts.includeDirs.push_back("include");
        dependency.artifacts.libDirs.push_back("lib");
        dependency.artifacts.libs.push_back("TinyCMakeLib");
        model.rootPackage.dependencies.push_back(dependency);

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics)) << diagnostics.diagnostics().dump();

        rhodium::LockFile lockFile;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", lockFile, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(lockFile.packages.size(), 1u);
        const auto &locked = lockFile.packages[0];
        EXPECT_EQ(locked.name, "TinyCMakeLib");
        EXPECT_EQ(locked.ref, "master");
        EXPECT_NE(locked.commit, "master");
        EXPECT_EQ(locked.commit.size(), 40u);
        EXPECT_FALSE(locked.buildHash.empty());
        EXPECT_EQ(locked.artifactId, "TinyCMakeLib/" + locked.commit + "-" + locked.buildHash);

        rhodium::PackageCache cache(dir / ".rhodium/packages");
        EXPECT_TRUE(rhodium::File::dir(cache.source(dependency)));
        EXPECT_FALSE(rhodium::File::dir(cache.path(dependency, locked.commit)));
        EXPECT_TRUE(rhodium::File::dir(cache.build(dependency, "debug-default")));
        const auto artifactPath = dir / ".rhodium" / "artifacts" / locked.artifactId;
        EXPECT_TRUE(rhodium::File::dir(artifactPath / "include"));
        EXPECT_TRUE(rhodium::File::dir(artifactPath / "lib"));
        EXPECT_TRUE(directoryHasFiles(artifactPath / "lib"));
        EXPECT_TRUE(rhodium::File::exists(artifactPath / "artifact.toml"));

        const auto lockText = rhodium::File::read(dir / "tpkg.lock.toml");
        EXPECT_EQ(lockText.find("cache_path"), std::string::npos);
        EXPECT_EQ(lockText.find(dir.generic_string()), std::string::npos);

        rhodium::JsonDiagnosticSink secondDiagnostics;
        ASSERT_TRUE(resolver.restore(model, dir, secondDiagnostics)) << secondDiagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(dir / "tpkg.lock.toml"), lockText);
        const auto secondLog = secondDiagnostics.diagnostics().dump();
        EXPECT_NE(secondLog.find("skipping git sync and build"), std::string::npos);
        EXPECT_EQ(secondLog.find("fetching package"), std::string::npos);
        EXPECT_EQ(secondLog.find("cloning package"), std::string::npos);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, FailedGitPkgReusesSourceCheckoutOnNextRestore)
    {
        const auto dir = makeTempDir();
        const auto repo = dir / "BrokenPrebuilt";
        writeText(repo / "README.md", "broken prebuilt\n");

        std::string processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "init"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "checkout", "-B", "master"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "config", "user.email", "broken@example.invalid"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "config", "user.name", "Broken Test"}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "add", "."}, processOutput)) << processOutput;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "commit", "-m", "init broken prebuilt"}, processOutput)) << processOutput;

        rhodium::BuildModel model;
        rhodium::DependencyDesc dependency;
        dependency.name = "BrokenPrebuilt";
        dependency.source = "BrokenPrebuilt";
        dependency.sourceType = "git";
        dependency.ref = "master";
        dependency.buildType = "prebuilt";
        dependency.artifacts.includeDirs.push_back("missing-include");
        model.rootPackage.dependencies.push_back(dependency);

        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink firstDiagnostics;
        ASSERT_FALSE(resolver.restore(model, dir, firstDiagnostics, "windows-msvc-x64", "debug"));

        rhodium::PackageCache cache(dir / ".rhodium/packages");
        const auto sourcePath = cache.source(dependency);
        EXPECT_TRUE(rhodium::File::dir(sourcePath / ".git"));
        EXPECT_EQ(directoryCount(cache.sourceRoot(dependency)), 1u);

        const auto buildDir = cache.build(dependency, "debug-windows-msvc-x64");
        writeText(buildDir / "stale.txt", "stale build cache\n");

        rhodium::JsonDiagnosticSink secondDiagnostics;
        ASSERT_FALSE(resolver.restore(model, dir, secondDiagnostics, "windows-msvc-x64", "debug"));
        EXPECT_TRUE(rhodium::File::dir(sourcePath / ".git"));
        EXPECT_EQ(directoryCount(cache.sourceRoot(dependency)), 1u);
        EXPECT_EQ(secondDiagnostics.diagnostics().dump().find("cloning package"), std::string::npos);
        EXPECT_TRUE(rhodium::File::exists(buildDir / "stale.txt"));

        rhodium::DependencyRestoreOptions rebuild;
        rebuild.rebuild = true;
        rhodium::JsonDiagnosticSink rebuildDiagnostics;
        ASSERT_FALSE(resolver.restore(model, dir, rebuildDiagnostics, "windows-msvc-x64", "debug", {}, rebuild));
        EXPECT_FALSE(rhodium::File::exists(buildDir / "stale.txt"));

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, ArchiveDependencyRestoresHeaderOnlyPackage)
    {
        const auto dir = makeTempDir();
        std::filesystem::create_directories(dir / "third_party");
        const auto archivePath = dir / "third_party" / "hello-lib.zip";
        createHelloZip(archivePath);

        rhodium::BuildModel model;
        rhodium::DependencyDesc dependency;
        dependency.name = "hello-lib";
        dependency.source = "third_party/hello-lib.zip";
        dependency.sourceType = "archive";
        dependency.sha256 = rhodium::sha256File(archivePath);
        dependency.stripComponents = 1;
        dependency.buildType = "header_only";
        dependency.artifacts.includeDirs.push_back("include");
        model.rootPackage.dependencies.push_back(dependency);

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug")) << diagnostics.diagnostics().dump();

        rhodium::LockFile lockFile;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", lockFile, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(lockFile.packages.size(), 1u);
        const auto &locked = lockFile.packages[0];
        EXPECT_EQ(locked.sourceType, "archive");
        EXPECT_EQ(locked.sha256, dependency.sha256);
        EXPECT_EQ(locked.resolvedArchiveHash, dependency.sha256);
        EXPECT_EQ(locked.stripComponents, 1);
        EXPECT_FALSE(locked.buildHash.empty());
        EXPECT_TRUE(rhodium::File::exists(dir / ".rhodium" / "artifacts" / locked.artifactId / "include" / "hello" / "hello.hpp"));

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RestoreDebugReleaseDoesNotOverwrite)
    {
        const auto dir = makeTempDir();
        std::filesystem::create_directories(dir / "third_party/HeaderOnly/include");

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        const auto model = localHeaderOnlyModel();
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug")) << diagnostics.diagnostics().dump();
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "release")) << diagnostics.diagnostics().dump();

        rhodium::LockFile lockFile;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", lockFile, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(lockFile.packages.size(), 2u);
        EXPECT_NE(lockFile.packages[0].config, lockFile.packages[1].config);
        EXPECT_NE(lockFile.packages[0].artifactId, lockFile.packages[1].artifactId);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RestoreDifferentToolchainsDoesNotOverwrite)
    {
        const auto dir = makeTempDir();
        std::filesystem::create_directories(dir / "third_party/HeaderOnly/include");

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        const auto model = localHeaderOnlyModel();
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug")) << diagnostics.diagnostics().dump();
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "android-clang-arm64", "debug")) << diagnostics.diagnostics().dump();

        rhodium::LockFile lockFile;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", lockFile, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(lockFile.packages.size(), 2u);
        EXPECT_NE(lockFile.packages[0].toolchainId, lockFile.packages[1].toolchainId);
        EXPECT_NE(lockFile.packages[0].artifactId, lockFile.packages[1].artifactId);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RestoreOverrideAndNormalCanCoexist)
    {
        const auto dir = makeTempDir();
        std::filesystem::create_directories(dir / "third_party/HeaderOnly/include");
        std::filesystem::create_directories(dir / "override/HeaderOnly/include");

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        const auto model = localHeaderOnlyModel();
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug")) << diagnostics.diagnostics().dump();

        rhodium::DependencyOverrideSet overrides;
        overrides.overrides["HeaderOnly"].path = dir / "override/HeaderOnly";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug", overrides)) << diagnostics.diagnostics().dump();

        rhodium::LockFile lockFile;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", lockFile, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(lockFile.packages.size(), 2u);
        EXPECT_NE(lockFile.packages[0].overridden, lockFile.packages[1].overridden);
        EXPECT_NE(lockFile.packages[0].artifactId, lockFile.packages[1].artifactId);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RestoreLockedMissingEntryFails)
    {
        const auto dir = makeTempDir();
        auto model = localHeaderOnlyModel();
        std::filesystem::create_directories(dir / "third_party" / "HeaderOnly" / "include");

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        rhodium::DependencyRestoreOptions options;
        options.locked = true;
        EXPECT_FALSE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug", {}, options));
        EXPECT_NE(diagnostics.diagnostics().dump().find("restore --locked requires an existing lock entry"), std::string::npos);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RestoreLockedDoesNotRewriteLockfile)
    {
        const auto dir = makeTempDir();
        auto model = localHeaderOnlyModel();
        std::filesystem::create_directories(dir / "third_party" / "HeaderOnly" / "include");

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        rhodium::DependencyRestoreOptions options;
        options.toolchainContentHash = "toolchain-a";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug", {}, options)) << diagnostics.diagnostics().dump();
        const auto before = rhodium::File::read(dir / "tpkg.lock.toml");

        rhodium::DependencyRestoreOptions locked;
        locked.locked = true;
        locked.toolchainContentHash = "toolchain-a";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug", {}, locked)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(dir / "tpkg.lock.toml"), before);

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, UpdatePackageOnlyUpdatesSpecifiedPackage)
    {
        const auto dir = makeTempDir();
        auto model = localHeaderOnlyModel("A");
        auto second = model.rootPackage.dependencies[0];
        second.name = "B";
        second.source = "third_party/B";
        model.rootPackage.dependencies.push_back(second);
        std::filesystem::create_directories(dir / "third_party" / "A" / "include");
        std::filesystem::create_directories(dir / "third_party" / "B" / "include");

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        rhodium::DependencyRestoreOptions firstOptions;
        firstOptions.toolchainContentHash = "toolchain-a";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug", {}, firstOptions)) << diagnostics.diagnostics().dump();

        rhodium::LockFile before;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", before, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(before.packages.size(), 2u);

        rhodium::DependencyRestoreOptions updateOptions;
        updateOptions.updatePackage = "A";
        updateOptions.toolchainContentHash = "toolchain-b";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "windows-msvc-x64", "debug", {}, updateOptions)) << diagnostics.diagnostics().dump();

        rhodium::LockFile after;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", after, diagnostics)) << diagnostics.diagnostics().dump();
        ASSERT_EQ(after.packages.size(), 2u);
        const auto findHash = [](const rhodium::LockFile &lock, const std::string &name) {
            for (const auto &package : lock.packages)
            {
                if (package.name == name)
                {
                    return package.buildHash;
                }
            }
            return std::string{};
        };
        EXPECT_NE(findHash(before, "A"), findHash(after, "A"));
        EXPECT_EQ(findHash(before, "B"), findHash(after, "B"));

        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, LockedRestoreRejectsChangedInputsWithoutPublishingArtifacts)
    {
        const auto dir = makeTempDir();
        auto model = localHeaderOnlyModel();
        std::filesystem::create_directories(dir / "third_party/HeaderOnly/include");
        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyRestoreOptions options;
        options.toolchainContentHash = "toolchain-a";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, "fixture", "debug", {}, options));
        const auto before = rhodium::File::read(dir / "tpkg.lock.toml");
        const auto artifactCount = directoryCount(dir / ".rhodium/artifacts/HeaderOnly");

        options.locked = true;
        model.rootPackage.dependencies[0].buildOptions["FEATURE"] = "ON";
        rhodium::JsonDiagnosticSink changedInputs;
        EXPECT_FALSE(resolver.restore(model, dir, changedInputs, "fixture", "debug", {}, options));
        EXPECT_NE(changedInputs.diagnostics().dump().find("inputs differ from the lock entry"), std::string::npos);
        EXPECT_EQ(rhodium::File::read(dir / "tpkg.lock.toml"), before);
        EXPECT_EQ(directoryCount(dir / ".rhodium/artifacts/HeaderOnly"), artifactCount);

        model.rootPackage.dependencies[0].buildOptions.clear();
        options.toolchainContentHash = "toolchain-b";
        rhodium::JsonDiagnosticSink changedToolchain;
        EXPECT_FALSE(resolver.restore(model, dir, changedToolchain, "fixture", "debug", {}, options));
        EXPECT_NE(changedToolchain.diagnostics().dump().find("inputs differ from the lock entry"), std::string::npos);
        EXPECT_EQ(rhodium::File::read(dir / "tpkg.lock.toml"), before);
        EXPECT_EQ(directoryCount(dir / ".rhodium/artifacts/HeaderOnly"), artifactCount);
        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, CachedArchiveHonorsExplicitStageOptions)
    {
        const auto dir = makeTempDir();
        createHelloZip(dir / "hello.zip");
        rhodium::BuildModel model;
        rhodium::DependencyDesc dependency;
        dependency.name = "hello";
        dependency.source = "hello.zip";
        dependency.sourceType = "archive";
        dependency.sha256 = rhodium::sha256File(dir / "hello.zip");
        dependency.stripComponents = 1;
        dependency.buildType = "header_only";
        dependency.artifacts.includeDirs = {"include"};
        model.rootPackage.dependencies.push_back(dependency);
        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics));
        rhodium::PackageCache cache(dir / ".rhodium/packages");
        const auto sentinel = cache.build(dependency, "debug-default") / "stale.txt";
        for (int mode = 0; mode < 3; ++mode)
        {
            writeText(sentinel, "old cache");
            rhodium::DependencyRestoreOptions options;
            options.rebuild = mode == 0;
            options.buildOnly = mode == 1;
            options.exportOnly = mode == 2;
            rhodium::JsonDiagnosticSink explicitStage;
            ASSERT_TRUE(resolver.restore(model, dir, explicitStage, {}, "debug", {}, options))
                << explicitStage.diagnostics().dump();
            EXPECT_EQ(explicitStage.diagnostics().dump().find("skipping archive build"), std::string::npos);
            EXPECT_EQ(explicitStage.diagnostics().dump().find("skipping build"), std::string::npos);
            if (options.rebuild)
            {
                EXPECT_FALSE(std::filesystem::exists(sentinel));
            }
        }
        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, GitBranchUpdatesUseRemoteTipAndKeepNestedSourceCache)
    {
        const auto dir = makeTempDir();
        const auto repo = dir / "BranchSource";
        writeText(repo / "include/value.hpp", "first\n");
        std::string output;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "init"}, output)) << output;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "checkout", "-B", "feature/topic"}, output)) << output;
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "config", "user.email", "test@example.invalid"}, output));
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "config", "user.name", "Fixture"}, output));
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "add", "."}, output));
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "commit", "-m", "first"}, output)) << output;

        rhodium::BuildModel model;
        rhodium::DependencyDesc dependency;
        dependency.name = "BranchPackage";
        dependency.source = "BranchSource";
        dependency.sourceType = "git";
        dependency.ref = "feature/topic";
        dependency.buildType = "header_only";
        dependency.artifacts.includeDirs = {"include"};
        model.rootPackage.dependencies.push_back(dependency);
        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics)) << diagnostics.diagnostics().dump();
        rhodium::LockFile first;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", first, diagnostics));
        ASSERT_EQ(first.packages.size(), 1u);
        rhodium::PackageCache cache(dir / ".rhodium/packages");
        ASSERT_TRUE(std::filesystem::is_directory(cache.source(dependency) / ".git"));

        const auto defaultWorkspace = dir / "default-consumer";
        auto defaultModel = model;
        defaultModel.rootPackage.dependencies[0].source = "../BranchSource";
        defaultModel.rootPackage.dependencies[0].ref.clear();
        ASSERT_TRUE(resolver.restore(defaultModel, defaultWorkspace, diagnostics)) << diagnostics.diagnostics().dump();

        for (int mode = 0; mode < 3; ++mode)
        {
            const auto sentinel = cache.build(dependency, "debug-default") / "stale.txt";
            writeText(sentinel, "old cache");
            rhodium::DependencyRestoreOptions options;
            options.rebuild = mode == 0;
            options.buildOnly = mode == 1;
            options.exportOnly = mode == 2;
            rhodium::JsonDiagnosticSink explicitStage;
            ASSERT_TRUE(resolver.restore(model, dir, explicitStage, {}, "debug", {}, options)) << explicitStage.diagnostics().dump();
            EXPECT_EQ(explicitStage.diagnostics().dump().find("skipping git sync and build"), std::string::npos);
            if (options.rebuild)
            {
                EXPECT_FALSE(std::filesystem::exists(sentinel));
            }
        }

        writeText(repo / "include/value.hpp", "second\n");
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "add", "."}, output));
        ASSERT_TRUE(runRequiredProcess("git", {"-C", repo.string(), "commit", "-m", "second"}, output)) << output;
        rhodium::DependencyRestoreOptions update;
        update.updatePackage = dependency.name;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, {}, "debug", {}, update)) << diagnostics.diagnostics().dump();
        rhodium::LockFile updated;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", updated, diagnostics));
        ASSERT_EQ(updated.packages.size(), 1u);
        EXPECT_NE(updated.packages[0].commit, first.packages[0].commit);
        EXPECT_EQ(rhodium::File::read(dir / ".rhodium/artifacts" / updated.packages[0].artifactId / "include/value.hpp"), "second\n");
        EXPECT_TRUE(std::filesystem::is_directory(cache.source(dependency) / ".git"));

        ASSERT_TRUE(resolver.restore(defaultModel, defaultWorkspace, diagnostics, {}, "debug", {}, update))
            << diagnostics.diagnostics().dump();
        rhodium::LockFile updatedDefault;
        ASSERT_TRUE(rhodium::LockFile::read(defaultWorkspace / "tpkg.lock.toml", updatedDefault, diagnostics));
        ASSERT_EQ(updatedDefault.packages.size(), 1u);
        EXPECT_EQ(updatedDefault.packages[0].commit, updated.packages[0].commit);

        model.rootPackage.dependencies[0].ref = first.packages[0].commit;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics)) << diagnostics.diagnostics().dump();
        rhodium::LockFile pinned;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", pinned, diagnostics));
        EXPECT_EQ(pinned.packages.back().commit, first.packages[0].commit);
        EXPECT_EQ(rhodium::File::read(dir / ".rhodium/artifacts" / pinned.packages.back().artifactId / "include/value.hpp"), "first\n");
        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, RejectsConflictingRestoreModesBeforeCreatingState)
    {
        const auto dir = makeTempDir();
        rhodium::BuildModel model;
        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyRestoreOptions options;
        options.locked = true;
        options.updateAll = true;
        EXPECT_FALSE(resolver.restore(model, dir, diagnostics, {}, "debug", {}, options));
        options = {};
        options.exportOnly = true;
        options.rebuild = true;
        EXPECT_FALSE(resolver.restore(model, dir, diagnostics, {}, "debug", {}, options));
        EXPECT_FALSE(std::filesystem::exists(dir / ".rhodium"));
        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, FullRestorePrunesRemovedDependenciesButTargetedRestorePreservesThem)
    {
        const auto dir = makeTempDir();
        auto model = localHeaderOnlyModel("A");
        auto second = model.rootPackage.dependencies[0];
        second.name = "B";
        second.source = "third_party/B";
        model.rootPackage.dependencies.push_back(second);
        std::filesystem::create_directories(dir / "third_party/A/include");
        std::filesystem::create_directories(dir / "third_party/B/include");
        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics)) << diagnostics.diagnostics().dump();
        rhodium::LockFile before;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", before, diagnostics));
        ASSERT_EQ(before.packages.size(), 2u);
        const auto oldArtifact = dir / ".rhodium/artifacts" / before.packages[1].artifactId;

        model.rootPackage.dependencies.pop_back();
        rhodium::DependencyRestoreOptions targeted;
        targeted.packageName = "A";
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, {}, "debug", {}, targeted));
        rhodium::LockFile partial;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", partial, diagnostics));
        EXPECT_EQ(partial.packages.size(), 2u);
        EXPECT_TRUE(std::filesystem::exists(oldArtifact));

        ASSERT_TRUE(resolver.restore(model, dir, diagnostics)) << diagnostics.diagnostics().dump();
        rhodium::LockFile after;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", after, diagnostics));
        ASSERT_EQ(after.packages.size(), 1u);
        EXPECT_EQ(after.packages[0].name, "A");
        EXPECT_FALSE(std::filesystem::exists(oldArtifact));
        EXPECT_TRUE(std::filesystem::is_directory(dir / "third_party/B/include"));
        std::filesystem::remove_all(dir);
    }

    TEST(DependencyResolverTests, PruningRemovedDependenciesPreservesOtherConfigurationVariants)
    {
        const auto dir = makeTempDir();
        auto model = localHeaderOnlyModel();
        std::filesystem::create_directories(dir / "third_party/HeaderOnly/include");
        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::DependencyResolver resolver;
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, {}, "debug"));
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, {}, "release"));
        model.rootPackage.dependencies.clear();
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, {}, "debug"));
        rhodium::LockFile remaining;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", remaining, diagnostics));
        ASSERT_EQ(remaining.packages.size(), 1u);
        EXPECT_EQ(remaining.packages[0].config, "release");
        ASSERT_TRUE(resolver.restore(model, dir, diagnostics, {}, "release"));
        rhodium::LockFile empty;
        ASSERT_TRUE(rhodium::LockFile::read(dir / "tpkg.lock.toml", empty, diagnostics));
        EXPECT_TRUE(empty.packages.empty());
        EXPECT_TRUE(std::filesystem::is_directory(dir / "third_party/HeaderOnly/include"));
        std::filesystem::remove_all(dir);
    }

} // namespace
