#include "rhodium/cli/Commands.hpp"
#include "rhodium/config/DependencyOverrideFile.hpp"
#include "rhodium/config/JsonModelWriter.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/script/ManifestLoader.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    class WorkspaceCompatibilityTests : public ::testing::Test
    {
    protected:
        std::filesystem::path root;

        void SetUp() override
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            root = std::filesystem::temp_directory_path() / ("rhodium-compatibility-" + std::to_string(stamp));
            std::filesystem::create_directories(root);
        }

        void TearDown() override
        {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
        }

        void writeManifest(const char *name, const char *text = "package(\"example\")\n")
        {
            std::ofstream out(root / name);
            out << text;
        }
    };

    TEST_F(WorkspaceCompatibilityTests, NewWorkspaceUsesRhodiumNames)
    {
        EXPECT_EQ(rhodium::CliWorkspace::manifest(root), root / "rhodium.lua");
        EXPECT_EQ(rhodium::Path::stateRoot(root), root / ".rhodium");
    }

    TEST_F(WorkspaceCompatibilityTests, LegacyManifestAndStateRemainUsable)
    {
        writeManifest("tpkg.lua");
        std::filesystem::create_directories(root / ".tpkg");
        EXPECT_EQ(rhodium::CliWorkspace::manifest(root), root / "tpkg.lua");
        EXPECT_EQ(rhodium::Path::stateRoot(root), root / ".tpkg");
        EXPECT_EQ(rhodium::DependencyOverrides::path(root), root / ".tpkg" / "overrides.toml");

        rhodium::ManifestLoader loader;
        rhodium::JsonDiagnosticSink diagnostics;
        const auto loaded = loader.load(rhodium::CliWorkspace::manifest(root), diagnostics);
        ASSERT_TRUE(loaded.ok) << diagnostics.diagnostics().dump();
        EXPECT_EQ(rhodium::JsonModel::from(loaded.model)["manifest"], "tpkg.lua");
    }

    TEST_F(WorkspaceCompatibilityTests, RhodiumNamesTakePrecedenceWhenBothExist)
    {
        writeManifest("tpkg.lua", "package(\"legacy\")\n");
        writeManifest("rhodium.lua", "package(\"preferred\")\n");
        std::filesystem::create_directories(root / ".tpkg");
        std::filesystem::create_directories(root / ".rhodium");
        EXPECT_EQ(rhodium::Path::stateRoot(root), root / ".rhodium");
        EXPECT_EQ(rhodium::DependencyOverrides::path(root), root / ".rhodium" / "overrides.toml");

        rhodium::ManifestLoader loader;
        rhodium::JsonDiagnosticSink diagnostics;
        const auto loaded = loader.load(rhodium::CliWorkspace::manifest(root), diagnostics);
        ASSERT_TRUE(loaded.ok) << diagnostics.diagnostics().dump();
        EXPECT_EQ(loaded.model.rootPackage.name, "preferred");
        EXPECT_EQ(rhodium::JsonModel::from(loaded.model)["manifest"], "rhodium.lua");
    }

    TEST_F(WorkspaceCompatibilityTests, NewManifestReusesAnExistingLegacyCache)
    {
        writeManifest("rhodium.lua");
        std::filesystem::create_directories(root / ".tpkg");
        EXPECT_EQ(rhodium::CliWorkspace::manifest(root), root / "rhodium.lua");
        EXPECT_EQ(rhodium::Path::stateRoot(root), root / ".tpkg");
    }

    TEST_F(WorkspaceCompatibilityTests, InvalidPreferredManifestDoesNotFallBackToLegacy)
    {
        writeManifest("tpkg.lua");
        writeManifest("rhodium.lua", "this is invalid Lua\n");
        rhodium::ManifestLoader loader;
        rhodium::JsonDiagnosticSink diagnostics;
        EXPECT_FALSE(loader.load(rhodium::CliWorkspace::manifest(root), diagnostics).ok);
    }

    TEST_F(WorkspaceCompatibilityTests, CleaningOnlyRemovesTheSelectedStateDirectory)
    {
        std::filesystem::create_directories(root / ".tpkg");
        std::filesystem::create_directories(root / ".rhodium");
        EXPECT_EQ(rhodium::Commands::clean(root, true, false, false, false, ""), 0);
        EXPECT_FALSE(std::filesystem::exists(root / ".rhodium"));
        EXPECT_TRUE(std::filesystem::exists(root / ".tpkg"));
    }

#ifdef _WIN32
    TEST_F(WorkspaceCompatibilityTests, CleaningRemovesDescendantsBeyondMaxPath)
    {
        const auto state = root / ".rhodium";
        const auto nested = state / std::string(100, 'a') / std::string(100, 'b');
        const auto extended = std::filesystem::path(L"\\\\?\\" + nested.native());
        std::filesystem::create_directories(extended);
        const auto file = extended / "long-cache-entry.txt";
        ASSERT_GT(file.native().size(), 260u);
        {
            std::ofstream out(file);
            ASSERT_TRUE(out.is_open());
            out << "cached input";
        }
        ASSERT_TRUE(std::filesystem::exists(file));
        const auto forwardSlashRoot = std::filesystem::path(root.generic_string());
        EXPECT_EQ(rhodium::Commands::clean(forwardSlashRoot, true, false, false, false, ""), 0);
        EXPECT_FALSE(std::filesystem::exists(state));
    }
#endif
}
