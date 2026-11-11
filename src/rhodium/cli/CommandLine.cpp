/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/cli/CommandLine.hpp"

#include "rhodium/cli/Commands.hpp"
#include "rhodium/cli/CommandContext.hpp"
#include "rhodium/core/Environment.hpp"
#include "rhodium/core/Logger.hpp"

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <filesystem>
#include <string>

namespace rhodium
{
#ifndef RHODIUM_VERSION
#define RHODIUM_VERSION "0.0.0"
#endif

    std::string CmdLine::version()
    {
        return fmt::format("{}-{}-{}-{}", RHODIUM_VERSION, Environment::buildModeName(), Environment::compilerName(), Environment::hostPlatformName());
    }

    std::filesystem::path CmdLine::root(const std::string &value)
    {
        auto path = value.empty() ? std::filesystem::current_path() : std::filesystem::path(value);
        if (path.is_relative())
        {
            path = std::filesystem::current_path() / path;
        }
        return std::filesystem::weakly_canonical(path);
    }

    int CmdLine::run(int argc, char **argv)
    {
        CLI::App app{"Rhodium C/C++ dependency manager"};
        app.set_version_flag("--version", CmdLine::version());

        CommandContext context;
        app.add_option("--root", context.rootOption, "Workspace root containing rhodium.lua (or legacy tpkg.lua)")
            ->capture_default_str();

#ifndef NDEBUG
        auto *doctor = app.add_subcommand("doctor", "Check the local Rhodium runtime");
        doctor->add_flag("--sdk", context.doctorSdk, "Show detailed SDK/toolchain diagnostics");
        doctor->callback([&]
                         { context.exitCode = Commands::doctor(root(context.rootOption), context.doctorSdk); });
        app.add_subcommand("load", "Load rhodium.lua (or legacy tpkg.lua) and validate the dependency DSL")->callback([&]
                                                                                                     { context.exitCode = Commands::load(root(context.rootOption)); });
        app.add_subcommand("dump-model", "Load rhodium.lua (or legacy tpkg.lua) and print the dependency model JSON")->callback([&]
                                                                                                               { context.exitCode = Commands::dumpModel(root(context.rootOption)); });
#endif
        app.add_subcommand("deps", "Print dependencies declared in rhodium.lua (or legacy tpkg.lua)")->callback([&]
                                                                                               { context.exitCode = Commands::deps(root(context.rootOption)); });
        auto *generate = app.add_subcommand("generate", "Generate CMake dependency integration files");
        generate->add_option("--toolchain", context.generateToolchain, "Preferred local toolchain profile id");
        generate->add_option("--config", context.generateConfig, "Build config: debug or release");
        generate->add_option("--output", context.generateOutputDir, "Output directory for generated CMake files (default: active state directory/generated/cmake)");
        generate->add_option("--override", context.commandOverrides, "Override dependency source: name=path");
        generate->callback([&]
                           { context.exitCode = Commands::generate(root(context.rootOption), context.generateToolchain, context.generateConfig, context.generateOutputDir.empty() ? std::filesystem::path{} : std::filesystem::path(context.generateOutputDir), context.commandOverrides); });
        auto *restore = app.add_subcommand("restore", "Prepare package cache and lockfile");
        restore->add_option("package", context.restorePackage, "Only restore one dependency")->required(false);
        restore->add_option("--toolchain", context.restoreToolchain, "Preferred local toolchain profile id");
        restore->add_option("--config", context.restoreConfig, "Build config: debug or release");
        restore->add_option("--override", context.commandOverrides, "Override dependency source: name=path");
        restore->add_flag("--locked", context.restoreLocked, "Use lockfile entries without updating the lockfile");
        restore->add_flag("--build-only", context.restoreBuildOnly, "Build the package workspace without exporting artifacts");
        restore->add_flag("--export-only", context.restoreExportOnly, "Export artifacts from an existing package build workspace");
        restore->add_flag("--rebuild", context.restoreRebuild, "Remove the package build workspace before building");
        restore->callback([&]
                          { context.exitCode = Commands::restore(root(context.rootOption), context.restoreToolchain, context.restoreConfig, context.commandOverrides, context.restoreLocked, context.restorePackage, context.restoreBuildOnly, context.restoreExportOnly, context.restoreRebuild, {}, false); });
        auto *update = app.add_subcommand("update", "Update package lock entries");
        update->add_option("package", context.updatePackage, "Package name")->required(false);
        update->add_option("--toolchain", context.updateToolchain, "Preferred local toolchain profile id");
        update->add_option("--config", context.updateConfig, "Build config: debug or release");
        update->add_option("--override", context.commandOverrides, "Override dependency source: name=path");
        update->add_flag("--all", context.updateAll, "Update all non-overridden packages");
        update->callback([&]
                         { context.exitCode = Commands::update(root(context.rootOption), context.updatePackage, context.updateAll, context.updateToolchain, context.updateConfig, context.commandOverrides); });
        auto *packages = app.add_subcommand("packages", "Print restored package cache and artifact status");
        packages->add_option("--override", context.commandOverrides, "Override dependency source: name=path");
        packages->add_flag("--verbose", context.packagesVerbose, "Print package build hash and artifact validation details");
        auto *packagesGc = packages->add_subcommand("gc", "Garbage collect unreferenced package caches");
        packagesGc->add_flag("--apply", context.packagesGcApply, "Delete unreferenced artifacts");
        packagesGc->add_option("--package", context.packagesGcPackage, "Only consider artifacts for one package");
        packagesGc->add_flag("--builds", context.packagesGcBuilds, "Also delete stale package build caches");
        packagesGc->add_flag("--sources", context.packagesGcSources, "Also delete stale package source caches");
        packagesGc->add_option("--toolchain", context.packagesGcToolchain, "Preferred local toolchain profile id for build cache retention");
        packagesGc->add_option("--config", context.packagesGcConfig, "Build config for build cache retention: debug or release");
        packagesGc->callback([&]
                             { context.exitCode = Commands::packagesGc(root(context.rootOption), context.packagesGcApply, context.packagesGcPackage, context.packagesGcBuilds, context.packagesGcSources, context.packagesGcToolchain, context.packagesGcConfig); });
        packages->callback([&]
                           {
                                if (!packagesGc->parsed())
                                {
                                    context.exitCode = Commands::packages(root(context.rootOption), context.packagesVerbose, context.commandOverrides);
                                } });
        auto *tree = app.add_subcommand("tree", "Display dependency tree");
        tree->callback([&]
                       { context.exitCode = Commands::tree(root(context.rootOption)); });
        auto *clean = app.add_subcommand("clean", "Clean rhodium dependency cache and generated integration files");
        clean->add_option("package", context.cleanPackage, "Only clean one dependency")->required(false);
        clean->add_flag("--all", context.cleanAll, "Remove the active .rhodium or legacy .tpkg directory");
        clean->add_flag("--artifacts", context.cleanArtifacts, "Also remove artifacts from the active state directory");
        clean->add_flag("--sources", context.cleanSources, "Also remove the active source cache");
        clean->add_flag("--lock", context.cleanLock, "Also remove tpkg.lock.toml");
        clean->callback([&]
                        { context.exitCode = Commands::clean(root(context.rootOption), context.cleanAll, context.cleanArtifacts, context.cleanSources, context.cleanLock, context.cleanPackage); });

        auto *overrideCommand = app.add_subcommand("override", "Manage local dependency overrides");
        overrideCommand->require_subcommand(1);
        overrideCommand->add_subcommand("list", "List local dependency overrides")->callback([&]
                                                                                             { context.exitCode = Commands::overrideList(root(context.rootOption)); });
        auto *overrideSet = overrideCommand->add_subcommand("set", "Set a local dependency override");
        overrideSet->add_option("name", context.overrideName, "Dependency name")->required();
        overrideSet->add_option("path", context.overridePathValue, "Local dependency path")->required();
        overrideSet->callback([&]
                              { context.exitCode = Commands::overrideSet(root(context.rootOption), context.overrideName, context.overridePathValue); });
        auto *overrideRemove = overrideCommand->add_subcommand("remove", "Remove a local dependency override");
        overrideRemove->add_option("name", context.overrideName, "Dependency name")->required();
        overrideRemove->callback([&]
                                 { context.exitCode = Commands::overrideRemove(root(context.rootOption), context.overrideName); });
        overrideCommand->add_subcommand("clear", "Clear local dependency overrides")->callback([&]
                                                                                               { context.exitCode = Commands::overrideClear(root(context.rootOption)); });
        auto *sdk = app.add_subcommand("sdk", "Detect and inspect local SDK/toolchains");
        sdk->require_subcommand(1);
        sdk->add_subcommand("detect", "Detect SDK/toolchains and write toolchains/host.toml in the active state directory")->callback([&]
                                                                                                                { context.exitCode = Commands::sdkDetect(root(context.rootOption)); });
        auto *sdkList = sdk->add_subcommand("list", "List detected SDK/toolchains");
        sdkList->add_flag("--refresh", context.sdkListRefresh, "Refresh detected SDK/toolchains before listing");
        sdkList->add_option("--source", context.sdkListSource, "Filter source: auto, project-user, workspace, global-user");
        sdkList->callback([&]
                          { context.exitCode = Commands::sdkList(root(context.rootOption), context.sdkListRefresh, context.sdkListSource); });
        auto *sdkShow = sdk->add_subcommand("show", "Show a resolved toolchain profile");
        sdkShow->add_option("id", context.sdkShowToolchain, "Toolchain profile id")->required();
        sdkShow->callback([&]
                          { context.exitCode = Commands::sdkShow(root(context.rootOption), context.sdkShowToolchain); });
        auto *sdkAdd = sdk->add_subcommand("add", "Register a user toolchain profile");
        sdkAdd->add_option("id", context.sdkAddOptions.profile.id, "Toolchain profile id")->required();
        sdkAdd->add_option("--source", context.sdkAddOptions.source, "Writable source: project-user or global-user")->default_val("project-user");
        sdkAdd->add_flag("--force", context.sdkAddOptions.force, "Replace an existing toolchain with the same id");
        sdkAdd->add_option("--platform", context.sdkAddOptions.profile.platform, "Target platform");
        sdkAdd->add_option("--compiler-kind", context.sdkAddOptions.profile.compilerKind, "Compiler kind");
        sdkAdd->add_option("--sdk-kind", context.sdkAddOptions.profile.sdkKind, "SDK kind");
        sdkAdd->add_option("--host-arch", context.sdkAddOptions.profile.hostArch, "Host architecture");
        sdkAdd->add_option("--target-arch", context.sdkAddOptions.profile.targetArch, "Target architecture");
        sdkAdd->add_option("--target-triple", context.sdkAddOptions.profile.targetTriple, "Target triple");
        sdkAdd->add_option("--cc", context.sdkAddOptions.profile.compiler, "C compiler");
        sdkAdd->add_option("--cxx", context.sdkAddOptions.profile.cxxCompiler, "C++ compiler");
        sdkAdd->add_option("--linker", context.sdkAddOptions.profile.linker, "Linker");
        sdkAdd->add_option("--archiver", context.sdkAddOptions.profile.archiver, "Archiver");
        sdkAdd->add_option("--rc", context.sdkAddOptions.profile.resourceCompiler, "Resource compiler");
        sdkAdd->add_option("--mt", context.sdkAddOptions.profile.manifestTool, "Manifest tool");
        sdkAdd->add_option("--strip", context.sdkAddOptions.profile.strip, "Strip tool");
        sdkAdd->add_option("--ranlib", context.sdkAddOptions.profile.ranlib, "Ranlib tool");
        sdkAdd->add_option("--ninja", context.sdkAddOptions.profile.ninja, "Ninja executable");
        sdkAdd->add_option("--git", context.sdkAddOptions.profile.git, "Git executable");
        sdkAdd->add_option("--include", context.sdkAddOptions.profile.systemIncludeDirs, "System include directory");
        sdkAdd->add_option("--lib", context.sdkAddOptions.profile.systemLibDirs, "System library directory");
        sdkAdd->add_option("--bin", context.sdkAddOptions.profile.binaryDirs, "Binary directory");
        sdkAdd->add_option("--sysroot", context.sdkAddOptions.profile.sysroot, "SDK sysroot/root");
        sdkAdd->add_option("--android-sdk", context.sdkAddOptions.profile.androidSdkRoot, "Android SDK root");
        sdkAdd->add_option("--android-ndk", context.sdkAddOptions.profile.androidNdkRoot, "Android NDK root");
        sdkAdd->add_option("--android-api", context.sdkAddOptions.profile.androidApi, "Android API level");
        sdkAdd->add_option("--android-abi", context.sdkAddOptions.profile.androidAbi, "Android ABI");
        sdkAdd->add_option("--jdk", context.sdkAddOptions.profile.jdkRoot, "JDK root");
        sdkAdd->add_option("--adb", context.sdkAddOptions.profile.adb, "adb executable");
        sdkAdd->add_option("--java", context.sdkAddOptions.profile.java, "java executable");
        sdkAdd->add_option("--javac", context.sdkAddOptions.profile.javac, "javac executable");
        sdkAdd->add_option("--gradle", context.sdkAddOptions.profile.gradle, "gradle executable");
        sdkAdd->add_option("--env", context.sdkAddOptions.env, "Environment variable KEY=VALUE");
        sdkAdd->callback([&]
                         { context.exitCode = Commands::sdkAdd(root(context.rootOption), context.sdkAddOptions); });
        auto *sdkRemove = sdk->add_subcommand("remove", "Remove a user toolchain profile");
        sdkRemove->add_option("id", context.sdkRemoveToolchain, "Toolchain profile id")->required();
        sdkRemove->add_option("--source", context.sdkRemoveSource, "Writable source: project-user or global-user")->default_val("project-user");
        sdkRemove->callback([&]
                            { context.exitCode = Commands::sdkRemove(root(context.rootOption), context.sdkRemoveToolchain, context.sdkRemoveSource); });
        sdk->add_subcommand("doctor", "Run SDK/toolchain diagnostics")->callback([&]
                                                                                 { context.exitCode = printSdkDoctor(root(context.rootOption)) ? 0 : 1; });
        sdk->add_subcommand("dump", "Dump cached SDK/toolchain TOML")->callback([&]
                                                                                { context.exitCode = Commands::sdkDump(root(context.rootOption)); });
        auto *sdkSelect = sdk->add_subcommand("select", "Save preferred local toolchain in the active state directory/local.toml");
        sdkSelect->add_option("id", context.sdkSelectToolchain, "Toolchain profile id")->required();
        sdkSelect->callback([&]
                            { context.exitCode = Commands::sdkSelect(root(context.rootOption), context.sdkSelectToolchain); });
        sdk->add_subcommand("clear", "Clear preferred local toolchain")->callback([&]
                                                                                  { context.exitCode = Commands::sdkClear(root(context.rootOption)); });
        app.require_subcommand(0, 1);

        try
        {
            app.parse(argc, argv);
            if (app.get_subcommands().empty())
            {
                LogInfo("{}", app.help());
            }
            return context.exitCode;
        }
        catch (const CLI::ParseError &ex)
        {
            return app.exit(ex);
        }
    }


} // namespace rhodium
