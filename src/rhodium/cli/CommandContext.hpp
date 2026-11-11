/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/toolchain/Toolchain.hpp"

#include <string>
#include <vector>

namespace rhodium
{
    struct SdkAddOptions
    {
        ToolchainProfile profile;
        std::string source = "project-user";
        bool force = false;
        std::vector<std::string> env;
    };

    struct CommandContext
    {
        int exitCode = 0;
        std::string rootOption;
        std::string restorePackage;
        std::string restoreToolchain;
        std::string restoreConfig;
        std::string generateToolchain;
        std::string generateConfig;
        std::string generateOutputDir;
        std::string updatePackage;
        std::string updateToolchain;
        std::string updateConfig;
        std::string sdkSelectToolchain;
        std::string sdkListSource;
        std::string sdkShowToolchain;
        std::string sdkRemoveToolchain;
        std::string sdkRemoveSource = "project-user";
        SdkAddOptions sdkAddOptions;
        std::string overrideName;
        std::string overridePathValue;
        std::vector<std::string> commandOverrides;
        bool doctorSdk = false;
        bool sdkListRefresh = false;
        bool restoreLocked = false;
        bool restoreBuildOnly = false;
        bool restoreExportOnly = false;
        bool restoreRebuild = false;
        bool updateAll = false;
        bool packagesVerbose = false;
        bool packagesGcApply = false;
        bool packagesGcBuilds = false;
        bool packagesGcSources = false;
        bool cleanAll = false;
        bool cleanArtifacts = false;
        bool cleanSources = false;
        bool cleanLock = false;
        std::string cleanPackage;
        std::string packagesGcPackage;
        std::string packagesGcToolchain;
        std::string packagesGcConfig;
    };
}
