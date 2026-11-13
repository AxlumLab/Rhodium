/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "Toolchain.hpp"
#include "ToolchainSerializer.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/core/Hash.hpp"

#include <filesystem>
#include <sstream>

namespace rhodium
{
    std::string Toolchain::contentHash(const ToolchainProfile &profile, const std::string &config)
    {
        std::ostringstream stream;
        ToolchainSerializer::appendString(stream, "config", BuildConfig::normalize(config));
        ToolchainSerializer::appendString(stream, "id", profile.id);
        ToolchainSerializer::appendString(stream, "platform", profile.platform);
        ToolchainSerializer::appendString(stream, "compiler_kind", profile.compilerKind);
        ToolchainSerializer::appendString(stream, "compiler_version", profile.compilerVersion);
        ToolchainSerializer::appendString(stream, "target_arch", profile.targetArch);
        ToolchainSerializer::appendString(stream, "target_triple", profile.targetTriple);
        ToolchainSerializer::appendString(stream, "sdk_kind", profile.sdkKind);
        ToolchainSerializer::appendString(stream, "sdk_version", profile.sdkVersion);
        ToolchainSerializer::appendString(stream, "android_api", profile.androidApi);
        ToolchainSerializer::appendString(stream, "android_abi", profile.androidAbi);
        ToolchainSerializer::appendString(stream, "deployment_target", profile.deploymentTarget);
        ToolchainSerializer::appendPath(stream, "compiler", profile.compiler);
        ToolchainSerializer::appendPath(stream, "cxx_compiler", profile.cxxCompiler);
        ToolchainSerializer::appendPath(stream, "linker", profile.linker);
        ToolchainSerializer::appendPath(stream, "archiver", profile.archiver);
        ToolchainSerializer::appendPath(stream, "sdk_root", profile.sdkRoot);
        ToolchainSerializer::appendPath(stream, "sysroot", profile.sysroot);
        ToolchainSerializer::appendPath(stream, "android_sdk_root", profile.androidSdkRoot);
        ToolchainSerializer::appendPath(stream, "android_ndk_root", profile.androidNdkRoot);
        return Hash::xxhash64Hex(stream.str());
    }
}