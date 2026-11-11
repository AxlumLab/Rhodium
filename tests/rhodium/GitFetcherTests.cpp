#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/package/GitFetcher.hpp"
#include "rhodium/package/ArtifactGc.hpp"
#include "rhodium/package/PackageCache.hpp"
#include "rhodium/resolve/DependencyLockStep.hpp"
#include "rhodium/resolve/DependencyResolver.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    class GitFetcherTests : public ::testing::Test
    {
    protected:
        std::filesystem::path root;
        std::filesystem::path source;
        rhodium::DependencyDesc dependency;
        std::string output;

        bool git(const std::filesystem::path &repo, const std::vector<std::string> &args)
        {
            std::vector<std::string> command{"-c", "core.hooksPath=/dev/null", "-c", "commit.gpgsign=false", "-C", repo.string()};
            command.insert(command.end(), args.begin(), args.end());
            const auto result = rhodium::Process::run("git", command);
            output = result.output;
            return result.exitCode == 0;
        }

        void SetUp() override
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            root = std::filesystem::temp_directory_path() / ("rhodium-git-fetch-" + std::to_string(stamp));
            source = root / "source";
            ASSERT_TRUE(rhodium::File::write(source / "value.txt", "first"));
            ASSERT_TRUE(git(source, {"init"})) << output;
            ASSERT_TRUE(git(source, {"checkout", "-B", "main"})) << output;
            ASSERT_TRUE(git(source, {"config", "user.name", "Fixture"}));
            ASSERT_TRUE(git(source, {"config", "user.email", "test@example.invalid"}));
            ASSERT_TRUE(git(source, {"add", "."}));
            ASSERT_TRUE(git(source, {"commit", "-m", "first"})) << output;
            dependency.name = "Example";
            dependency.source = source.string();
            dependency.sourceType = "git";
            dependency.ref = "main";
        }

        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(root, ignored);
        }
    };

    TEST_F(GitFetcherTests, FreshCloneDoesNotImmediatelyFetchTheSameRemoteAgain)
    {
        rhodium::GitFetcher fetcher;
        rhodium::JsonDiagnosticSink diagnostics;
        std::string commit;
        ASSERT_TRUE(fetcher.fetch(dependency, root / "cache", commit, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_EQ(commit.size(), 40u);
        EXPECT_EQ(diagnostics.diagnostics().dump().find("fetching package"), std::string::npos);
        EXPECT_EQ(rhodium::File::read(root / "cache/value.txt"), "first");
    }

    TEST_F(GitFetcherTests, CachedCommitCanBeRestoredWithoutAWorkingRemote)
    {
        rhodium::GitFetcher fetcher;
        rhodium::JsonDiagnosticSink first;
        std::string commit;
        ASSERT_TRUE(fetcher.fetch(dependency, root / "cache", commit, first)) << first.diagnostics().dump();
        ASSERT_TRUE(git(root / "cache", {"remote", "set-url", "origin", (root / "missing").string()}));
        dependency.ref = commit;
        rhodium::JsonDiagnosticSink cached;
        std::string restored;
        ASSERT_TRUE(fetcher.fetch(dependency, root / "cache", restored, cached)) << cached.diagnostics().dump();
        EXPECT_EQ(restored, commit);
        EXPECT_EQ(cached.diagnostics().dump().find("fetching package"), std::string::npos);
        rhodium::JsonDiagnosticSink refresh;
        EXPECT_FALSE(fetcher.fetch(dependency, root / "cache", restored, refresh, true));
    }

    TEST_F(GitFetcherTests, MovingBranchesStillFetchAndPinnedCommitsRemainImmutable)
    {
        rhodium::GitFetcher fetcher;
        rhodium::JsonDiagnosticSink diagnostics;
        std::string first;
        ASSERT_TRUE(fetcher.fetch(dependency, root / "cache", first, diagnostics));
        ASSERT_TRUE(rhodium::File::write(source / "value.txt", "second"));
        ASSERT_TRUE(git(source, {"add", "."}));
        ASSERT_TRUE(git(source, {"commit", "-m", "second"})) << output;
        std::string second;
        ASSERT_TRUE(fetcher.fetch(dependency, root / "cache", second, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_NE(first, second);
        EXPECT_EQ(rhodium::File::read(root / "cache/value.txt"), "second");
        dependency.ref = first;
        std::string pinned;
        ASSERT_TRUE(fetcher.fetch(dependency, root / "cache", pinned, diagnostics));
        EXPECT_EQ(pinned, first);
        EXPECT_EQ(rhodium::File::read(root / "cache/value.txt"), "first");
    }

    TEST_F(GitFetcherTests, SubmoduleFetchFailureFailsTheWholeSourceRestore)
    {
        const auto child = root / "child";
        ASSERT_TRUE(rhodium::File::write(child / "child.txt", "child"));
        ASSERT_TRUE(git(child, {"init"}));
        ASSERT_TRUE(git(child, {"config", "user.name", "Fixture"}));
        ASSERT_TRUE(git(child, {"config", "user.email", "test@example.invalid"}));
        ASSERT_TRUE(git(child, {"add", "."}));
        ASSERT_TRUE(git(child, {"commit", "-m", "child"})) << output;
        ASSERT_TRUE(git(source, {"-c", "protocol.file.allow=always", "submodule", "add", child.string(), "vendor/child"})) << output;
        ASSERT_TRUE(git(source, {"config", "-f", ".gitmodules", "submodule.vendor/child.url", (root / "missing-submodule").string()}));
        ASSERT_TRUE(git(source, {"add", "."}));
        ASSERT_TRUE(git(source, {"commit", "-m", "add broken submodule URL"})) << output;

        rhodium::GitFetcher fetcher;
        rhodium::JsonDiagnosticSink diagnostics;
        std::string commit;
        EXPECT_FALSE(fetcher.fetch(dependency, root / "cache", commit, diagnostics));
        const auto text = diagnostics.diagnostics().dump();
        EXPECT_NE(text.find("failed to update submodule"), std::string::npos);
        EXPECT_EQ(text.find("restored git package"), std::string::npos);
    }

    TEST_F(GitFetcherTests, SourceGcPreservesAbsoluteLocalSourcesButPrunesTheirUnusedRefs)
    {
        rhodium::GitFetcher fetcher;
        rhodium::PackageCache cache(root / ".rhodium/packages");
        rhodium::JsonDiagnosticSink diagnostics;
        std::string commit;
        const auto live = cache.source(dependency);
        const auto stale = cache.path(dependency, "stale-ref");
        ASSERT_TRUE(fetcher.fetch(dependency, live, commit, diagnostics));
        ASSERT_TRUE(fetcher.fetch(dependency, stale, commit, diagnostics));
        auto package = rhodium::LockedPackage::from(dependency, commit, "debug", "");
        package.source = rhodium::DepLock::source(root, dependency);
        rhodium::LockFile lock;
        lock.packages.push_back(package);
        ASSERT_TRUE(rhodium::ArtifactGc::runSourceCaches(lock, root, true, {}, diagnostics)) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(std::filesystem::is_directory(live / ".git"));
        EXPECT_FALSE(std::filesystem::exists(stale));
    }

    TEST_F(GitFetcherTests, AbsoluteManifestSourcesUseTheExistingLockEntryOnWarmRestore)
    {
        ASSERT_TRUE(rhodium::File::write(source / "include/value.hpp", "header"));
        ASSERT_TRUE(git(source, {"add", "."}));
        ASSERT_TRUE(git(source, {"commit", "-m", "add header"})) << output;
        dependency.buildType = "header_only";
        dependency.artifacts.includeDirs = {"include"};
        rhodium::BuildModel model;
        model.rootPackage.dependencies.push_back(dependency);
        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink first;
        ASSERT_TRUE(resolver.restore(model, root, first)) << first.diagnostics().dump();
        rhodium::JsonDiagnosticSink warm;
        ASSERT_TRUE(resolver.restore(model, root, warm)) << warm.diagnostics().dump();
        EXPECT_NE(warm.diagnostics().dump().find("skipping git sync and build"), std::string::npos);
        rhodium::PackageCache cache(root / ".rhodium/packages");
        EXPECT_TRUE(std::filesystem::is_directory(cache.source(dependency) / ".git"));
    }

    TEST_F(GitFetcherTests, RestoringAnotherConfigurationDoesNotMoveItsReferencedSourceCheckout)
    {
        ASSERT_TRUE(rhodium::File::write(source / "include/value.hpp", "header"));
        ASSERT_TRUE(git(source, {"add", "."}));
        ASSERT_TRUE(git(source, {"commit", "-m", "add header"})) << output;
        ASSERT_TRUE(git(source, {"tag", "release"}));
        dependency.buildType = "header_only";
        dependency.artifacts.includeDirs = {"include"};
        rhodium::BuildModel model;
        model.rootPackage.dependencies.push_back(dependency);
        rhodium::DependencyResolver resolver;
        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(resolver.restore(model, root, diagnostics, {}, "debug"));
        rhodium::PackageCache cache(root / ".rhodium/packages");
        const auto debugSource = cache.source(dependency);
        model.rootPackage.dependencies[0].ref = "release";
        ASSERT_TRUE(resolver.restore(model, root, diagnostics, {}, "release")) << diagnostics.diagnostics().dump();
        EXPECT_TRUE(std::filesystem::is_directory(debugSource / ".git"));
        EXPECT_TRUE(std::filesystem::is_directory(cache.source(model.rootPackage.dependencies[0]) / ".git"));
    }
}
