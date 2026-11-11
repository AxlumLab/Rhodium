/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include "rhodium/package/PackageArtifact.hpp"

#include <sstream>
#include <toml++/toml.hpp>
#include <vector>

namespace rhodium
{

    class DiagnosticSink;

    class ArtifactStore
    {
    public:
        static bool write(const PackageArtifact &artifact, DiagnosticSink &diagnostics);
        static bool read(const std::filesystem::path &path,
                         DependencyArtifacts &artifacts,
                         DiagnosticSink &diagnostics);

    private:
        static void writeArray(std::ostringstream &stream, const char *key, const std::vector<std::string> &values);
        static std::vector<std::string> readArray(const toml::table &table, const char *key);
    };

} // namespace rhodium
