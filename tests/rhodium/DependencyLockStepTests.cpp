#include "rhodium/resolve/DependencyLockStep.hpp"

#include <gtest/gtest.h>

namespace
{
    rhodium::LockedPackage entry(const std::string &ref, const std::string &config = "debug", const std::string &toolchain = "one")
    {
        rhodium::DependencyDesc dependency;
        dependency.name = "Example";
        dependency.source = "https://example.invalid/source.git";
        dependency.sourceType = "git";
        dependency.ref = ref;
        return rhodium::LockedPackage::from(dependency, ref, config, toolchain);
    }

    TEST(DependencyLockStepTests, ReplacesObsoleteSourcesWithoutLosingOtherBuildVariants)
    {
        rhodium::LockFile lock;
        lock.packages = {entry("v1"), entry("v0"), entry("v1", "release"), entry("v1", "debug", "two")};
        auto overridden = entry("override");
        overridden.overridden = true;
        overridden.overridePath = "/local/override";
        lock.packages.push_back(overridden);
        auto current = entry("v2");
        current.source = "https://example.invalid/replacement.git";
        rhodium::DepLock::store(lock, current);
        ASSERT_EQ(lock.packages.size(), 4u);
        EXPECT_EQ(lock.packages[0].ref, "v2");
        EXPECT_EQ(lock.packages[0].source, current.source);
        EXPECT_EQ(lock.packages[1].config, "release");
        EXPECT_EQ(lock.packages[2].toolchainId, "two");
        EXPECT_TRUE(lock.packages[3].overridden);
    }

    TEST(DependencyLockStepTests, MigratesLegacyToolchainEntriesAndRemovesTheirDuplicates)
    {
        rhodium::LockFile lock;
        lock.packages = {entry("v1", "debug", ""), entry("v0"), entry("v1", "release", "")};
        const auto *legacy = &lock.packages[0];
        rhodium::DepLock::store(lock, entry("v2"), legacy);
        ASSERT_EQ(lock.packages.size(), 2u);
        EXPECT_EQ(lock.packages[0].toolchainId, "one");
        EXPECT_EQ(lock.packages[0].ref, "v2");
        EXPECT_EQ(lock.packages[1].config, "release");
    }
}
