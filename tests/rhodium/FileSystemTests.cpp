/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/FileSystem.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

namespace rhodium::tests::fileSystemTests
{
    class FileSystemTests : public ::testing::Test
    {
    protected:
        std::filesystem::path root;
        void SetUp() override
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            root = std::filesystem::temp_directory_path() / ("rhodium-files-" + std::to_string(stamp));
            std::filesystem::create_directories(root);
        }
        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(root, ignored);
        }
    };

    TEST_F(FileSystemTests, ReplacesExistingContentsAndLeavesNoTemporaryFiles)
    {
        std::string error;
        ASSERT_TRUE(rhodium::File::write(root / "state.toml", "old contents", &error)) << error;
        ASSERT_TRUE(rhodium::File::write(root / "state.toml", "new", &error)) << error;
        EXPECT_EQ(rhodium::File::read(root / "state.toml"), "new");
        ASSERT_TRUE(rhodium::File::write(root / "state.toml", "", &error)) << error;
        EXPECT_EQ(std::filesystem::file_size(root / "state.toml"), 0u);
        std::size_t count = 0;
        for (const auto &entry : std::filesystem::directory_iterator(root))
        {
            (void)entry;
            ++count;
        }
        EXPECT_EQ(count, 1u);
    }

    TEST_F(FileSystemTests, FailedReplacementDoesNotRemoveTheExistingDestination)
    {
        std::filesystem::create_directory(root / "destination");
        ASSERT_TRUE(rhodium::File::write(root / "destination/keep", "preserved"));
        std::string error;
        EXPECT_FALSE(rhodium::File::write(root / "destination", "", &error));
        EXPECT_FALSE(error.empty());
        EXPECT_EQ(rhodium::File::read(root / "destination/keep"), "preserved");
        for (const auto &entry : std::filesystem::directory_iterator(root))
        {
            EXPECT_EQ(entry.path().filename(), "destination");
        }
    }

    TEST_F(FileSystemTests, WritesThroughAnExistingConfigurationSymlink)
    {
        ASSERT_TRUE(rhodium::File::write(root / "target", "old"));
        std::error_code ec;
        std::filesystem::create_symlink("target", root / "config", ec);
        if (ec)
        {
            GTEST_SKIP() << "symlinks are unavailable: " << ec.message();
        }
        std::string error;
        ASSERT_TRUE(rhodium::File::write(root / "config", "updated", &error)) << error;
        EXPECT_TRUE(std::filesystem::is_symlink(root / "config"));
        EXPECT_EQ(rhodium::File::read(root / "target"), "updated");
    }

#ifndef _WIN32
    TEST_F(FileSystemTests, PreservesPrivateFilePermissions)
    {
        ASSERT_TRUE(rhodium::File::write(root / "config", "old"));
        std::filesystem::permissions(root / "config",
                                     std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
        ASSERT_TRUE(rhodium::File::write(root / "config", "new"));
        EXPECT_EQ(std::filesystem::status(root / "config").permissions(),
                  std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    }

    TEST_F(FileSystemTests, ConcurrentReadersOnlySeeCompleteVersions)
    {
        const std::string first(65536, 'a');
        const std::string second(65536, 'b');
        ASSERT_TRUE(rhodium::File::write(root / "state", first));
        std::atomic<bool> finished{false};
        std::atomic<bool> torn{false};
        std::thread reader(
            [&]
            {
                while (!finished.load())
                {
                    const auto value = rhodium::File::read(root / "state");
                    if (value != first && value != second)
                    {
                        torn = true;
                    }
                }
            });
        bool written = true;
        for (int index = 0; index < 16 && written; ++index)
        {
            written = rhodium::File::write(root / "state", index % 2 ? first : second);
        }
        finished = true;
        reader.join();
        EXPECT_TRUE(written);
        EXPECT_FALSE(torn.load());
    }
#endif
} // namespace rhodium::tests::fileSystemTests
