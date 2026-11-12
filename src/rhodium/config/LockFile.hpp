/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include "rhodium/export.hpp"

#include "rhodium/model/DependencyDesc.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace rhodium
{
    class DiagnosticSink;

    struct RHODIUM_EXPORT LockedPackage
    {
        std::string name;
        std::string source;
        std::string sourceType = "git";
        std::string sha256;
        std::string resolvedArchiveHash;
        std::string ref;
        std::string commit;
        std::string subdir = ".";
        int stripComponents = 0;
        std::vector<std::string> patches;
        std::vector<std::string> patchHashes;
        std::string buildType = "header_only";
        std::string linkage = "default";
        std::string runtime = "default";
        std::string pic = "default";
        std::string config = "debug";
        std::string toolchainId;
        std::string buildHash;
        std::string artifactId;
        std::map<std::string, std::string> buildOptions;
        DependencyArtifacts artifacts;
        bool overridden = false;
        std::string overridePath;
        std::string overrideSource;
        std::string originalSource;
        std::string originalSourceType;
        std::string originalRef;
        bool exportDefines = false;  // Export defines to consuming projects

        static LockedPackage from(const DependencyDesc &dependency, const std::string &commit);
        static LockedPackage from(const DependencyDesc &dependency,
                                  const std::string &commit,
                                  const std::string &config,
                                  const std::string &toolchainId);
        static LockedPackage from(const DependencyDesc &dependency,
                                  const std::string &commit,
                                  const std::string &config,
                                  const std::string &toolchainId,
                                  const std::string &toolchainContentHash);
    };

    struct RHODIUM_EXPORT LockFile
    {
        int version = 1;
        std::vector<LockedPackage> packages;

        static bool read(const std::filesystem::path &path, LockFile &lockFile, DiagnosticSink &diagnostics);
        static bool write(const std::filesystem::path &path, const LockFile &lockFile, DiagnosticSink &diagnostics);
    };

} // namespace rhodium
