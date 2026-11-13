/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"
#include "rhodium/script/ManifestLoader.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace rhodium::tests::luaDslTests
{

    std::filesystem::path makeTempDir()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    TEST(LuaDslTests, LoadsDependencyOnlyManifest)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "rhodium.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("rhodium-deps")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")
cmake({ generate_user_presets = true })

require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    linkage = "static",
    runtime = "dynamic",
    pic = true,
    toolchain_requirements = {
        family = "msvc-abi",
        abi = "msvc",
        binary_format = "coff"
    },
    cmake = {
        generator = "Ninja",
        build_type = "Release",
        options = {
            FMT_TEST = false
        },
        configure_args = { "-DFMT_DOC=OFF" },
        build_args = { "--verbose" },
        install_args = { "--strip" },
        env = {
            CC = "clang"
        },
        toolchain_file = "$(source)/toolchain.cmake",
        install = true,
        install_target = "install"
    },
    artifacts = {
        mode = "install",
        include_dirs = { "include" },
        libs = { "fmt" },
        system_libs = { "user32" },
        frameworks = { "CoreFoundation" },
        lib_files = {
            debug = { "lib/fmtd.lib" },
            release = { "lib/fmt.lib" }
        }
    }
})

require_local("third_party/Local", {
    name = "Local",
    build = "header_only",
    artifacts = {
        includes = { "include" }
    }
})
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        ASSERT_TRUE(result.ok) << diagnostics.diagnostics().dump();
        EXPECT_EQ(result.model.rootPackage.name, "rhodium-deps");
        EXPECT_EQ(result.model.rootPackage.version, "0.1.0");
        EXPECT_EQ(result.model.workspace.defaultConfig, "debug");
        EXPECT_FALSE(result.model.workspace.defaultPlatform.empty());
        EXPECT_TRUE(result.model.workspace.generateCMakeUserPresets);

        ASSERT_EQ(result.model.rootPackage.dependencies.size(), 2u);
        EXPECT_EQ(result.model.rootPackage.dependencies[0].name, "fmt");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].source, "https://github.com/fmtlib/fmt.git");
        EXPECT_FALSE(result.model.rootPackage.dependencies[0].local);
        EXPECT_EQ(result.model.rootPackage.dependencies[0].buildType, "cmake");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].linkage, "static");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].runtime, "dynamic");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].pic, "true");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].toolchainRequirements.family, "msvc-abi");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].buildOptions["FMT_TEST"], "OFF");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].cmake.configureArgs[0], "-DFMT_DOC=OFF");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].artifacts.includeDirs[0], "include");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].artifacts.libs[0], "fmt");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].artifacts.libFilesByConfig["debug"][0], "lib/fmtd.lib");
        EXPECT_EQ(result.model.rootPackage.dependencies[0].artifacts.libFilesByConfig["release"][0], "lib/fmt.lib");
        EXPECT_EQ(result.model.rootPackage.dependencies[1].name, "Local");
        EXPECT_TRUE(result.model.rootPackage.dependencies[1].local);

        std::filesystem::remove_all(dir);
    }

    TEST(LuaDslTests, RejectsProjectPkgDsl)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "tpkg.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("bad")
module("Core")
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        EXPECT_FALSE(result.ok);
        EXPECT_NE(diagnostics.diagnostics().dump().find("project build DSL 'module' is not supported"),
                  std::string::npos);
        std::filesystem::remove_all(dir);
    }

    TEST(LuaDslTests, RejectsOtherProjectPkgDslEntryPoints)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "tpkg.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("bad")
target("App")
sources({ "main.cpp" })
public_deps({ "Core" })
private_deps({ "Impl" })
custom_command({ "echo bad" })
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        EXPECT_FALSE(result.ok);
        const auto text = diagnostics.diagnostics().dump();
        EXPECT_NE(text.find("project build DSL 'target' is not supported"), std::string::npos);
        EXPECT_NE(text.find("project build DSL 'sources' is not supported"), std::string::npos);
        EXPECT_NE(text.find("project build DSL 'public_deps' is not supported"), std::string::npos);
        EXPECT_NE(text.find("project build DSL 'private_deps' is not supported"), std::string::npos);
        EXPECT_NE(text.find("project build DSL 'custom_command' is not supported"), std::string::npos);
        std::filesystem::remove_all(dir);
    }

    TEST(LuaDslTests, AcceptsFlatLibFilesArtifactDsl)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "tpkg.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("rhodium-deps")

require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    artifacts = {
        include_dirs = { "include" },
        lib_files = { "lib/fmt.lib" }
    }
})
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        ASSERT_TRUE(result.ok) << diagnostics.diagnostics().dump();
        ASSERT_EQ(result.model.rootPackage.dependencies.size(), 1u);
        EXPECT_EQ(result.model.rootPackage.dependencies[0].artifacts.libFiles[0], "lib/fmt.lib");
        EXPECT_TRUE(result.model.rootPackage.dependencies[0].artifacts.libFilesByConfig.empty());
        std::filesystem::remove_all(dir);
    }

    TEST(LuaDslTests, LoadsPlatformCustomCompileBlock)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "tpkg.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("custom-compile")
default_platform("windows")

require("lua", {
    source = "https://github.com/lua/lua.git",
    ref = "v5.4.8",
    custom_compile = {
        windows = {
            commands = {
                build = { "cl onelua.c" },
                install = { "lib lua.lib" }
            },
            artifacts = {
                include_dirs = { "include" },
                lib_files = { "lib/lua.lib" }
            }
        }
    }
})
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        ASSERT_TRUE(result.ok) << diagnostics.diagnostics().dump();
        ASSERT_EQ(result.model.rootPackage.dependencies.size(), 1u);
        const auto &dependency = result.model.rootPackage.dependencies[0];
        EXPECT_EQ(dependency.name, "lua");
        EXPECT_EQ(dependency.buildType, "custom");
        ASSERT_EQ(dependency.commands.build.size(), 1u);
        EXPECT_EQ(dependency.commands.build[0], "cl onelua.c");
        ASSERT_EQ(dependency.artifacts.libFiles.size(), 1u);
        EXPECT_EQ(dependency.artifacts.libFiles[0], "lib/lua.lib");
        std::filesystem::remove_all(dir);
    }

    TEST(LuaDslTests, CustomCompileSupportsUnixFallback)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "tpkg.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("custom-compile")
default_platform("linux")

require("lua", {
    source = "https://github.com/lua/lua.git",
    ref = "v5.4.8",
    custom_compile = {
        unix = {
            commands = {
                build = { "cc onelua.c" }
            },
            artifacts = {
                include_dirs = { "include" },
                lib_files = { "lib/liblua.a" }
            }
        }
    }
})
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        ASSERT_TRUE(result.ok) << diagnostics.diagnostics().dump();
        ASSERT_EQ(result.model.rootPackage.dependencies.size(), 1u);
        const auto &dependency = result.model.rootPackage.dependencies[0];
        EXPECT_EQ(dependency.buildType, "custom");
        ASSERT_EQ(dependency.commands.build.size(), 1u);
        EXPECT_EQ(dependency.commands.build[0], "cc onelua.c");
        ASSERT_EQ(dependency.artifacts.libFiles.size(), 1u);
        EXPECT_EQ(dependency.artifacts.libFiles[0], "lib/liblua.a");
        std::filesystem::remove_all(dir);
    }

    TEST(LuaDslTests, WhenSupportsAndroidAndIosConditions)
    {
        const auto dir = makeTempDir();
        const auto manifest = dir / "tpkg.lua";

        std::ofstream out(manifest);
        out << R"lua(
package("mobile-conditions")
default_platform("android")

when("android", function()
    require_local(".", {
        name = "android-system",
        build = "prebuilt",
        artifacts = {
            system_libs = { "android", "log" }
        }
    })
end)

when("ios", function()
    require_local(".", {
        name = "ios-system",
        build = "prebuilt",
        artifacts = {
            frameworks = { "Foundation" }
        }
    })
end)
)lua";
        out.close();

        rhodium::JsonDiagnosticSink diagnostics;
        rhodium::ManifestLoader loader;
        auto result = loader.load(manifest, diagnostics);

        ASSERT_TRUE(result.ok) << diagnostics.diagnostics().dump();
        ASSERT_EQ(result.model.rootPackage.dependencies.size(), 1u);
        const auto &dependency = result.model.rootPackage.dependencies[0];
        EXPECT_EQ(dependency.name, "android-system");
        ASSERT_EQ(dependency.artifacts.systemLibs.size(), 2u);
        EXPECT_EQ(dependency.artifacts.systemLibs[0], "android");
        std::filesystem::remove_all(dir);
    }

} // namespace rhodium::tests::luaDslTests
