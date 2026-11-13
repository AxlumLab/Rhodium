/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */
#include "rhodium/cmake/CMakeDependencyExporter.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace rhodium::tests::cMakeDependencyExporterTests
{
    std::filesystem::path makeTempDir()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-cmake-deps-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    TEST(CMakeDependencyExporterTests, WritesFindPackageConfigAndImportedTargets)
    {
        const auto root = makeTempDir();
        const auto output = root / ".tpkg/generated/cmake";

        rhodium::ResolvedDependencyArtifact package;
        package.name = "fmt";
        package.includeDirs.push_back(root / "artifacts/fmt/include");
        const auto libDir = root / "artifacts/fmt/lib";
        std::filesystem::create_directories(libDir);
        std::ofstream(libDir / "fmt.lib").put('x');
        std::ofstream(libDir / "fmtd.lib").put('x');
        package.libDirs.push_back(libDir);
        package.libs.push_back("fmt");
        package.libFiles.push_back(libDir / "fmtd.lib");
        package.defines.push_back("FMT_HEADER_ONLY=0");
        package.systemLibs.push_back("pthread");

        rhodium::CMakeDependencyExportMetadata metadata;
        metadata.toolchainId = "windows-msvc-x64";
        metadata.compilerKind = "msvc";
        metadata.config = "debug";
        metadata.arch = "x64";
        metadata.runtime = "dynamic";
        metadata.linkage = "static";

        rhodium::JsonDiagnosticSink diagnostics;
        ASSERT_TRUE(rhodium::CMakeDependency::writeFiles({package}, output, metadata, diagnostics))
            << diagnostics.diagnostics().dump();

        const auto config = rhodium::File::read(output / "rhodiumConfig.cmake");
        EXPECT_NE(config.find("rhodiumTargets.cmake"), std::string::npos);

        const auto targets = rhodium::File::read(output / "rhodiumTargets.cmake");
        EXPECT_NE(targets.find("add_library(rhodium::fmt INTERFACE IMPORTED)"), std::string::npos);
        EXPECT_NE(targets.find("INTERFACE_INCLUDE_DIRECTORIES"), std::string::npos);
        EXPECT_EQ(targets.find("INTERFACE_LINK_DIRECTORIES"), std::string::npos);
        EXPECT_NE(targets.find("fmt.lib"), std::string::npos);
        EXPECT_NE(targets.find("fmtd.lib"), std::string::npos);
        EXPECT_NE(targets.find("\"pthread\""), std::string::npos);

        EXPECT_NE(config.find("RHODIUM_CONFIG"), std::string::npos);
        EXPECT_NE(config.find("RHODIUM_COMPILER_KIND"), std::string::npos);
        EXPECT_NE(config.find("CMAKE_CONFIGURATION_TYPES"), std::string::npos);
        EXPECT_NE(config.find("CMAKE_MSVC_RUNTIME_LIBRARY"), std::string::npos);

        std::filesystem::remove_all(root);
    }

    TEST(CMakeDependencyExporterTests, FailsWhenLogicalLibraryCannotBeResolved)
    {
        const auto root = makeTempDir();
        const auto output = root / ".tpkg/generated/cmake";
        const auto libDir = root / "artifacts/fmt/lib";
        std::filesystem::create_directories(libDir);

        rhodium::ResolvedDependencyArtifact package;
        package.name = "fmt";
        package.libDirs.push_back(libDir);
        package.libs.push_back("fmt");

        rhodium::JsonDiagnosticSink diagnostics;
        EXPECT_FALSE(rhodium::CMakeDependency::writeFiles({package}, output, diagnostics));

        const auto text = diagnostics.diagnostics().dump();
        EXPECT_NE(text.find("no matching library file was found"), std::string::npos);
        EXPECT_NE(text.find("searched"), std::string::npos);

        std::filesystem::remove_all(root);
    }
} // namespace rhodium::tests::cMakeDependencyExporterTests
