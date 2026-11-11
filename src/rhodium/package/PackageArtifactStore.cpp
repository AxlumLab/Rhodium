/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/PackageArtifactStore.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <toml++/toml.hpp>
#include <vector>

namespace rhodium
{
    void ArtifactStore::writeArray(std::ostringstream &stream, const char *key, const std::vector<std::string> &values)
    {
        stream << key << " = [";
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i != 0)
            {
                stream << ", ";
            }
            stream << toml::value<std::string>(values[i]);
        }
        stream << "]\n";
    }

    std::vector<std::string> ArtifactStore::readArray(const toml::table &table, const char *key)
    {
        std::vector<std::string> values;
        const auto *array = table[key].as_array();
        if (!array)
        {
            return values;
        }
        for (const auto &item : *array)
        {
            if (auto value = item.value<std::string>())
            {
                values.push_back(*value);
            }
        }
        return values;
    }

    bool ArtifactStore::write(const PackageArtifact &artifact, DiagnosticSink &diagnostics)
    {
        std::ostringstream stream;
        stream << "name = " << toml::value<std::string>(artifact.name) << "\n";
        stream << "commit = " << toml::value<std::string>(artifact.commit) << "\n";
        stream << "build_hash = " << toml::value<std::string>(artifact.buildHash) << "\n";
        stream << "artifact_id = " << toml::value<std::string>(artifact.artifactId) << "\n\n";
        stream << "[artifacts]\n";
        stream << "mode = " << toml::value<std::string>(artifact.artifacts.mode) << "\n";
        writeArray(stream, "include_dirs", artifact.artifacts.includeDirs);
        writeArray(stream, "lib_dirs", artifact.artifacts.libDirs);
        writeArray(stream, "bin_dirs", artifact.artifacts.binDirs);
        writeArray(stream, "bin_files", artifact.artifacts.binFiles);
        writeArray(stream, "libs", artifact.artifacts.libs);
        writeArray(stream, "lib_files", artifact.artifacts.libFiles);
        writeArray(stream, "defines", artifact.artifacts.defines);
        writeArray(stream, "system_libs", artifact.artifacts.systemLibs);
        writeArray(stream, "frameworks", artifact.artifacts.frameworks);

        std::string error;
        if (!File::write(artifact.root / "artifact.toml", stream.str(), &error))
        {
            diagnostics.error("failed to write package artifact metadata: " + error);
            return false;
        }
        return true;
    }

    bool ArtifactStore::read(const std::filesystem::path &path,
                             DependencyArtifacts &artifacts,
                             DiagnosticSink &diagnostics)
    {
        if (!File::exists(path))
        {
            diagnostics.error("missing package artifact metadata: " + path.string());
            return false;
        }

        try
        {
            auto table = toml::parse_file(path.string());
            const auto *artifactTable = table["artifacts"].as_table();
            if (!artifactTable)
            {
                diagnostics.error("package artifact metadata is missing [artifacts]: " + path.string());
                return false;
            }
            artifacts.mode = (*artifactTable)["mode"].value_or("install");
            artifacts.includeDirs = readArray(*artifactTable, "include_dirs");
            artifacts.libDirs = readArray(*artifactTable, "lib_dirs");
            artifacts.binDirs = readArray(*artifactTable, "bin_dirs");
            artifacts.binFiles = readArray(*artifactTable, "bin_files");
            artifacts.libs = readArray(*artifactTable, "libs");
            artifacts.libFiles = readArray(*artifactTable, "lib_files");
            artifacts.defines = readArray(*artifactTable, "defines");
            artifacts.systemLibs = readArray(*artifactTable, "system_libs");
            artifacts.frameworks = readArray(*artifactTable, "frameworks");
            return true;
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to read package artifact metadata: " + std::string(ex.what()));
            return false;
        }
    }

} // namespace rhodium
