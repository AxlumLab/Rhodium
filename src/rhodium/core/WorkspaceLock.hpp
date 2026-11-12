#pragma once

#include "rhodium/export.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

namespace rhodium
{
    class DiagnosticSink;

    // Serializes workspace operations across processes, threads, and nested stages.
    // The OS lock lives outside the cache, so clean --all cannot remove it.
    // Competing operations receive a busy error instead of waiting indefinitely.
    class RHODIUM_EXPORT WorkspaceLock
    {
    public:
        explicit WorkspaceLock(const std::filesystem::path &workspaceRoot, std::string *error = nullptr);
        ~WorkspaceLock();
        WorkspaceLock(const WorkspaceLock &) = delete;
        WorkspaceLock &operator=(const WorkspaceLock &) = delete;
        WorkspaceLock(WorkspaceLock &&) = delete;
        WorkspaceLock &operator=(WorkspaceLock &&) = delete;

        explicit operator bool() const
        {
            return held_;
        }

        static std::unique_ptr<WorkspaceLock> acquire(const std::filesystem::path &workspaceRoot,
                                                      DiagnosticSink &diagnostics, bool createRoot = false);

    private:
        struct State;
        std::shared_ptr<State> state_;
        std::unique_lock<std::recursive_mutex> threadLock_;
        bool held_ = false;
    };
}
