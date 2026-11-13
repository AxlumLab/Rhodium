/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/cli/Commands.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Process.hpp"
#include "rhodium/core/WorkspaceLock.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

namespace rhodium::tests::workspaceLockTests
{
    class WorkspaceLockTests : public ::testing::Test
    {
    protected:
        std::filesystem::path root;
        void SetUp() override
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            root = std::filesystem::temp_directory_path() / ("rhodium-lock-" + std::to_string(stamp));
            std::filesystem::create_directories(root);
        }
        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(root, ignored);
        }
        int probe() const
        {
            return rhodium::Process::run(RHODIUM_LOCK_PROBE, {root.string()}).exitCode;
        }
    };

    TEST_F(WorkspaceLockTests, IsReentrantAndExcludesOtherProcessesUntilTheOuterGuardExits)
    {
        std::string error;
        {
            rhodium::WorkspaceLock outer(root, &error);
            ASSERT_TRUE(static_cast<bool>(outer)) << error;
            {
                rhodium::WorkspaceLock inner(root / ".", &error);
                ASSERT_TRUE(static_cast<bool>(inner)) << error;
                EXPECT_EQ(probe(), 2);
            }
            EXPECT_EQ(probe(), 2);
        }
        EXPECT_EQ(probe(), 0);
    }

    TEST_F(WorkspaceLockTests, ReportsContentionFromAnotherThreadWithoutDeadlocking)
    {
        rhodium::WorkspaceLock outer(root);
        ASSERT_TRUE(static_cast<bool>(outer));
        bool acquired = true;
        std::string error;
        std::thread contender(
            [&]
            {
                rhodium::WorkspaceLock lock(root, &error);
                acquired = static_cast<bool>(lock);
            });
        contender.join();
        EXPECT_FALSE(acquired);
        EXPECT_NE(error.find("already in use"), std::string::npos);
    }

    TEST_F(WorkspaceLockTests, CleaningTheCacheDoesNotReleaseTheWorkspaceLock)
    {
        ASSERT_TRUE(rhodium::File::write(root / ".rhodium/tmp/sentinel", "temporary"));
        {
            rhodium::WorkspaceLock outer(root);
            ASSERT_TRUE(static_cast<bool>(outer));
            EXPECT_EQ(rhodium::Commands::clean(root, rhodium::CleanOptions{true, false, false, false, ""}), 0);
            EXPECT_FALSE(std::filesystem::exists(root / ".rhodium"));
            EXPECT_EQ(probe(), 2);
        }
        EXPECT_EQ(probe(), 0);
        EXPECT_TRUE(std::filesystem::is_empty(root));
    }
} // namespace rhodium::tests::workspaceLockTests
