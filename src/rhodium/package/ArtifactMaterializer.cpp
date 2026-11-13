/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/package/ArtifactMaterializer.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/ArtifactValidator.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    std::filesystem::path ArtifactFiles::input(const BuildUtil::Context &context, const std::string &value)
    {
        auto path = std::filesystem::path(value);
        if (path.is_absolute())
        {
            return path;
        }

        auto sourcePath = context.sourceDir / path;
        if (std::filesystem::exists(sourcePath))
        {
            return sourcePath;
        }

        auto buildPath = context.buildDir / path;
        if (std::filesystem::exists(buildPath))
        {
            return buildPath;
        }

        return sourcePath;
    }

    std::string ArtifactFiles::output(const std::string &value)
    {
        auto path = std::filesystem::path(value);
        if (path.is_absolute())
        {
            return path.filename().string();
        }
        return path.generic_string();
    }

    bool ArtifactFiles::copy(const BuildUtil::Context &context,
                             const std::vector<std::string> &requested,
                             std::vector<std::string> &exported,
                             const char *field,
                             DiagnosticSink &diagnostics)
    {
        for (const auto &value : requested)
        {
            if (value.empty())
            {
                diagnostics.error(std::string("package artifact ") + field + " path is empty");
                return false;
            }

            const auto inputPath = input(context, value);
            if (!std::filesystem::exists(inputPath))
            {
                diagnostics.error(std::string("package artifact ") + field + " path is missing: " + inputPath.string());
                return false;
            }

            const auto outputName = output(value);
            const auto outputPath = context.artifactDir / outputName;
            std::string error;
            if (!File::mkdir(outputPath.parent_path(), &error))
            {
                diagnostics.error("failed to create artifact copy parent: " + error);
                return false;
            }

            try
            {
                if (std::filesystem::is_directory(inputPath))
                {
                    if (std::filesystem::exists(outputPath))
                    {
                        std::filesystem::remove_all(outputPath);
                    }
                    std::filesystem::copy(inputPath,
                                          outputPath,
                                          std::filesystem::copy_options::recursive |
                                              std::filesystem::copy_options::overwrite_existing);
                }
                else
                {
                    std::filesystem::copy_file(inputPath, outputPath, std::filesystem::copy_options::overwrite_existing);
                }
            }
            catch (const std::exception &ex)
            {
                diagnostics.error(std::string("failed to copy package artifact ") + field + ": " + ex.what());
                return false;
            }

            exported.push_back(outputName);
        }
        return true;
    }

    bool ArtifactFiles::materialize(const BuildUtil::Context &context,
                                    const DependencyArtifacts &requested,
                                    DependencyArtifacts &exported,
                                    DiagnosticSink &diagnostics)
    {
        exported = requested;
        if (StringUtils::toLower(requested.mode) != "copy")
        {
            return ArtifactCheck::validate(context, exported, diagnostics);
        }

        exported.includeDirs.clear();
        exported.libDirs.clear();
        exported.binDirs.clear();
        exported.binFiles.clear();
        exported.libFiles.clear();
        if (!copy(context, requested.includeDirs, exported.includeDirs, "include_dirs", diagnostics) ||
            !copy(context, requested.libDirs, exported.libDirs, "lib_dirs", diagnostics) ||
            !copy(context, requested.binDirs, exported.binDirs, "bin_dirs", diagnostics) ||
            !copy(context, requested.binFiles, exported.binFiles, "bin_files", diagnostics) ||
            !copy(context, requested.libFiles, exported.libFiles, "lib_files", diagnostics))
        {
            return false;
        }

        return ArtifactCheck::validate(context, exported, diagnostics);
    }

} // namespace rhodium