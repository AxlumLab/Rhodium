#pragma once

#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <filesystem>
#include <functional>
#include <vector>

namespace rhodium
{
    enum class CopyStatus
    {
        Copied,
        Skipped,
        Failed,
    };

    struct CopiedFile
    {
        std::filesystem::path source;
        std::filesystem::path destination;
        CopyStatus status = CopyStatus::Skipped;
    };

    using CopyFilter = std::function<bool(const std::filesystem::path &)>;

    class FileCopy
    {
    public:
        static CopyStatus file(const std::filesystem::path &source,
                               const std::filesystem::path &destination,
                               DiagnosticSink &diagnostics);

        static bool dir(const std::filesystem::path &source,
                        const std::filesystem::path &destination,
                        DiagnosticSink &diagnostics,
                        std::vector<CopiedFile> *files = nullptr,
                        CopyFilter filter = {});

    private:
        static bool same(const std::filesystem::path &source, const std::filesystem::path &destination);
    };
}
