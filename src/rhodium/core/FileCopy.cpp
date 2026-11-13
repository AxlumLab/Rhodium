/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/FileCopy.hpp"

#include <system_error>

namespace rhodium
{
    bool FileCopy::same(const std::filesystem::path &source, const std::filesystem::path &destination)
    {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(destination, ec))
        {
            return false;
        }
        const auto sourceSize = std::filesystem::file_size(source, ec);
        if (ec)
        {
            return false;
        }
        const auto destinationSize = std::filesystem::file_size(destination, ec);
        if (ec || sourceSize != destinationSize)
        {
            return false;
        }
        const auto sourceTime = std::filesystem::last_write_time(source, ec);
        if (ec)
        {
            return false;
        }
        const auto destinationTime = std::filesystem::last_write_time(destination, ec);
        return !ec && sourceTime == destinationTime;
    }

    CopyStatus FileCopy::file(const std::filesystem::path &source,
                              const std::filesystem::path &destination,
                              DiagnosticSink &diagnostics)
    {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(source, ec))
        {
            diagnostics.error("copy source file is missing: " + source.string());
            return CopyStatus::Failed;
        }
        if (same(source, destination))
        {
            return CopyStatus::Skipped;
        }

        std::filesystem::create_directories(destination.parent_path(), ec);
        if (ec)
        {
            diagnostics.error("failed to create copy destination parent: " + destination.parent_path().string());
            return CopyStatus::Failed;
        }

        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
        {
            diagnostics.error("failed to copy file " + source.string() + " -> " + destination.string() + ": " + ec.message());
            return CopyStatus::Failed;
        }
        const auto sourceTime = std::filesystem::last_write_time(source, ec);
        if (!ec)
        {
            std::filesystem::last_write_time(destination, sourceTime, ec);
        }
        return CopyStatus::Copied;
    }

    bool FileCopy::dir(const std::filesystem::path &sourceDir,
                       const std::filesystem::path &destinationDir,
                       DiagnosticSink &diagnostics,
                       std::vector<CopiedFile> *files,
                       CopyFilter filter)
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(sourceDir, ec))
        {
            diagnostics.error("copy source directory is missing: " + sourceDir.string());
            return false;
        }

        for (const auto &entry : std::filesystem::recursive_directory_iterator(sourceDir, ec))
        {
            if (ec)
            {
                diagnostics.error("failed to enumerate copy source directory: " + sourceDir.string());
                return false;
            }
            if (!entry.is_regular_file())
            {
                continue;
            }
            const auto source = entry.path();
            if (filter && !filter(source))
            {
                continue;
            }
            const auto relative = std::filesystem::relative(source, sourceDir, ec);
            if (ec)
            {
                diagnostics.error("failed to compute relative copy path: " + source.string());
                return false;
            }
            const auto destination = destinationDir / relative;
            const auto status = file(source, destination, diagnostics);
            if (status == CopyStatus::Failed)
            {
                return false;
            }
            if (files)
            {
                files->push_back({source, destination, status});
            }
        }
        return true;
    }
}
