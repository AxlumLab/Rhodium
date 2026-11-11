#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/package/ToolchainContextResolver.hpp"
#include "rhodium/resolve/DependencyBuildStep.hpp"
#include "rhodium/toolchain/ToolchainRegistry.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace
{
    class DependencyBuildStepTests : public ::testing::Test
    {
    protected:
        std::filesystem::path root;
        std::string toolchainId;
        rhodium::DependencyDesc dependency;
        rhodium::DependencyRestoreOptions options;
        rhodium::JsonDiagnosticSink diagnostics;

        void SetUp() override
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            root = std::filesystem::temp_directory_path() / ("rhodium-build-stage-" + std::to_string(stamp));
            toolchainId = "rhodium-fixture-" + std::to_string(stamp);
            ASSERT_TRUE(rhodium::File::write(root / "source/input.txt", "first", nullptr));
            ASSERT_TRUE(rhodium::File::write(root / "dummy-tool", "unused", nullptr));
            rhodium::ToolchainProfile profile;
            profile.id = toolchainId;
            profile.platform = "linux";
            profile.compilerKind = "gcc";
            profile.compiler = profile.cxxCompiler = profile.linker = profile.archiver = profile.ninja = root / "dummy-tool";
            ASSERT_TRUE(rhodium::writeToolchainProfiles(root / "config/toolchains.toml", {profile}, diagnostics));

            dependency.name = "Example";
            dependency.source = "https://example.invalid/example.git";
            dependency.sourceType = "git";
            dependency.ref = "main";
            dependency.buildType = "custom";
            dependency.artifacts.mode = "copy";
            dependency.artifacts.binFiles = {"built.txt"};
#ifdef _WIN32
            dependency.commands.build = {R"cmd(copy /Y "$(source)\input.txt" "$(build)\built.txt" >nul)cmd"};
#else
            dependency.commands.build = {"cp '$(source)/input.txt' '$(build)/built.txt'"};
#endif
        }

        void TearDown() override
        {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
        }

        std::filesystem::path buildDir() const
        {
            return root / ".rhodium/build-packages/Example" / ("debug-" + toolchainId);
        }

        bool run(const std::string &commit, rhodium::DepBuild::Result &result)
        {
            rhodium::PackageCache cache(root / ".rhodium/packages");
            const std::string config = "debug";
            const std::string toolchain = toolchainId;
            rhodium::DepBuild::Context context{root, cache, config, toolchain, options};
            return rhodium::DepBuild::run(dependency, root / "source", commit, nullptr, false, context, result, diagnostics);
        }
    };

    TEST_F(DependencyBuildStepTests, ChangedCommitAndOptionsInvalidateTheBuildFingerprint)
    {
        rhodium::DepBuild::Result first;
        ASSERT_TRUE(run("commit-a", first)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(first.artifactDir / "built.txt"), "first");
        EXPECT_EQ(rhodium::File::read(buildDir() / ".tpkg_build_complete"), first.updated.buildHash + "\n");

        ASSERT_TRUE(rhodium::File::write(root / "source/input.txt", "second", nullptr));
        rhodium::DepBuild::Result unchanged;
        ASSERT_TRUE(run("commit-a", unchanged)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(unchanged.artifactDir / "built.txt"), "first");

        ASSERT_TRUE(rhodium::File::write(buildDir() / "stale-option", "old", nullptr));
        rhodium::DepBuild::Result changedCommit;
        ASSERT_TRUE(run("commit-b", changedCommit)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(changedCommit.artifactDir / "built.txt"), "second");
        EXPECT_FALSE(std::filesystem::exists(buildDir() / "stale-option"));

        ASSERT_TRUE(rhodium::File::write(root / "source/input.txt", "third", nullptr));
        dependency.buildOptions["FEATURE"] = "ON";
        rhodium::DepBuild::Result changedOptions;
        ASSERT_TRUE(run("commit-b", changedOptions)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(changedOptions.artifactDir / "built.txt"), "third");
        EXPECT_NE(changedOptions.updated.buildHash, changedCommit.updated.buildHash);

        ASSERT_TRUE(rhodium::File::write(root / "source/input.txt", "fourth", nullptr));
        options.toolchainContentHash = "changed-toolchain";
        rhodium::DepBuild::Result changedToolchain;
        ASSERT_TRUE(run("commit-b", changedToolchain)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(changedToolchain.artifactDir / "built.txt"), "fourth");
    }

    TEST_F(DependencyBuildStepTests, LocalEditsReenterTheBuildWithoutDiscardingIncrementalState)
    {
        dependency.sourceType = "local";
        dependency.source = "source";
        // The source type is authoritative even when the convenience flag is unset.
        dependency.local = false;
        rhodium::DepBuild::Result result;
        ASSERT_TRUE(run("local", result)) << diagnostics.diagnostics().dump();
        ASSERT_TRUE(rhodium::File::write(root / "source/input.txt", "edited", nullptr));
        ASSERT_TRUE(rhodium::File::write(buildDir() / "incremental-state", "keep", nullptr));
        ASSERT_TRUE(run("local", result)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(result.artifactDir / "built.txt"), "edited");
        EXPECT_TRUE(std::filesystem::exists(buildDir() / "incremental-state"));
    }

    TEST_F(DependencyBuildStepTests, LegacyMarkersRequireOneFreshBuild)
    {
        ASSERT_TRUE(rhodium::File::write(buildDir() / ".tpkg_build_complete", "built\n", nullptr));
        ASSERT_TRUE(rhodium::File::write(buildDir() / "built.txt", "stale", nullptr));
        rhodium::DepBuild::Result result;
        ASSERT_TRUE(run("commit-a", result)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::File::read(result.artifactDir / "built.txt"), "first");
    }

    TEST_F(DependencyBuildStepTests, ExportOnlyRejectsStaleInputsWithoutDiscardingTheBuild)
    {
        rhodium::DepBuild::Result result;
        ASSERT_TRUE(run("commit-a", result)) << diagnostics.diagnostics().dump();
        const auto marker = rhodium::File::read(buildDir() / ".tpkg_build_complete");
        options.exportOnly = true;
        EXPECT_FALSE(run("commit-b", result));
        EXPECT_EQ(rhodium::File::read(buildDir() / ".tpkg_build_complete"), marker);
        EXPECT_EQ(rhodium::File::read(buildDir() / "built.txt"), "first");
        ASSERT_TRUE(run("commit-a", result)) << diagnostics.diagnostics().dump();
    }

    TEST_F(DependencyBuildStepTests, FailedLocalBuildInvalidatesItsPreviousCompletionMarker)
    {
        dependency.sourceType = "local";
        dependency.source = "source";
        dependency.local = true;
        rhodium::DepBuild::Result result;
        ASSERT_TRUE(run("local", result)) << diagnostics.diagnostics().dump();
        std::filesystem::remove(root / "source/input.txt");
        EXPECT_FALSE(run("local", result));
        EXPECT_FALSE(std::filesystem::exists(buildDir() / ".tpkg_build_complete"));
        options.exportOnly = true;
        EXPECT_FALSE(run("local", result));
    }

    TEST_F(DependencyBuildStepTests, OneRestoreSnapshotSurvivesChangesToTheToolchainConfiguration)
    {
        rhodium::ToolchainSnapshot snapshot(root);
        options.toolchains = &snapshot;
        rhodium::DepBuild::Result result;
        ASSERT_TRUE(run("commit-a", result)) << diagnostics.diagnostics().dump();
        ASSERT_TRUE(rhodium::writeToolchainProfiles(root / "config/toolchains.toml", {}, diagnostics));
        dependency.name = "Second";
        ASSERT_TRUE(run("commit-a", result)) << diagnostics.diagnostics().dump();

        rhodium::ToolchainSnapshot nextRestore(root);
        options.toolchains = &nextRestore;
        dependency.name = "Third";
        EXPECT_FALSE(run("commit-a", result));
        EXPECT_NE(diagnostics.diagnostics().dump().find("requested toolchain was not detected: " + toolchainId), std::string::npos);
    }
}
