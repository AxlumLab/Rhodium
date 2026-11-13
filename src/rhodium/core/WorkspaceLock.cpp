/*
 * @Author: AxlumLab
 * @LastEditors: AxlumLab
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/WorkspaceLock.hpp"

#include "rhodium/core/Hash.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <algorithm>
#include <cerrno>
#include <cwctype>
#include <iterator>
#include <map>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace rhodium
{
    struct WorkspaceLock::State
    {
        std::recursive_mutex mutex;
        unsigned depth = 0;
#ifdef _WIN32
        HANDLE handle = nullptr;
#else
        int descriptor = -1;
#endif
    };

    WorkspaceLock::WorkspaceLock(const std::filesystem::path &workspaceRoot, std::string *error)
    {
        if (error)
        {
            error->clear();
        }
        auto fail = [&](const std::string &message)
        {
            if (error)
            {
                *error = message;
            }
        };
        try
        {
            const auto root =
                std::filesystem::canonical(workspaceRoot.empty() ? std::filesystem::path(".") : workspaceRoot);
            if (!std::filesystem::is_directory(root))
            {
                fail("workspace root is not a directory: " + root.string());
                return;
            }
            auto identity = root;
#ifdef _WIN32
            auto spelling = root.native();
            std::transform(spelling.begin(), spelling.end(), spelling.begin(),
                           [](wchar_t value)
                           {
                               return static_cast<wchar_t>(std::towlower(value));
                           });
            identity = std::filesystem::path(spelling);
#endif
            const auto utf8 = identity.generic_u8string();
            const std::string key(reinterpret_cast<const char *>(utf8.data()), utf8.size());
            static std::mutex registryMutex;
            static std::map<std::string, std::weak_ptr<State>> registry;
            {
                std::lock_guard<std::mutex> registryLock(registryMutex);
                for (auto entry = registry.begin(); entry != registry.end();)
                {
                    entry = entry->second.expired() ? registry.erase(entry) : std::next(entry);
                }
                state_ = registry[key].lock();
                if (!state_)
                {
                    state_ = std::make_shared<State>();
                    registry[key] = state_;
                }
            }
            threadLock_ = std::unique_lock<std::recursive_mutex>(state_->mutex, std::try_to_lock);
            if (!threadLock_.owns_lock())
            {
                fail("workspace is already in use by another operation");
                return;
            }
            if (state_->depth == 0)
            {
#ifdef _WIN32
                const auto name =
                    std::wstring(L"Global\\RhodiumWorkspace-") + std::filesystem::path(Hash::xxhash64Hex(key)).native();
                state_->handle = CreateMutexW(nullptr, FALSE, name.c_str());
                if (!state_->handle)
                {
                    fail("failed to create workspace mutex: " + std::system_category().message(GetLastError()));
                    return;
                }
                const DWORD result = WaitForSingleObject(state_->handle, 0);
                if (result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
                {
                    const auto code = GetLastError();
                    CloseHandle(state_->handle);
                    state_->handle = nullptr;
                    fail(result == WAIT_TIMEOUT
                             ? "workspace is already in use by another process"
                             : "failed to acquire workspace mutex: " + std::system_category().message(code));
                    return;
                }
#else
                state_->descriptor = open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
                if (state_->descriptor == -1)
                {
                    fail("failed to open workspace for locking: " + std::generic_category().message(errno));
                    return;
                }
                int result;
                do
                {
                    result = flock(state_->descriptor, LOCK_EX | LOCK_NB);
                } while (result != 0 && errno == EINTR);
                if (result != 0)
                {
                    const auto code = errno;
                    close(state_->descriptor);
                    state_->descriptor = -1;
                    fail(code == EWOULDBLOCK || code == EAGAIN
                             ? "workspace is already in use by another process"
                             : "failed to acquire workspace lock: " + std::generic_category().message(code));
                    return;
                }
#endif
            }
            ++state_->depth;
            held_ = true;
        }
        catch (const std::exception &ex)
        {
            fail(ex.what());
        }
    }

    WorkspaceLock::~WorkspaceLock()
    {
        if (held_ && --state_->depth == 0)
        {
#ifdef _WIN32
            ReleaseMutex(state_->handle);
            CloseHandle(state_->handle);
            state_->handle = nullptr;
#else
            flock(state_->descriptor, LOCK_UN);
            close(state_->descriptor);
            state_->descriptor = -1;
#endif
        }
    }

    std::unique_ptr<WorkspaceLock> WorkspaceLock::acquire(const std::filesystem::path &workspaceRoot,
                                                          DiagnosticSink &diagnostics, bool createRoot)
    {
        std::string error;
        if (createRoot && !File::mkdir(workspaceRoot, &error))
        {
            diagnostics.error("failed to create workspace root: " + error);
            return {};
        }
        auto lock = std::make_unique<WorkspaceLock>(workspaceRoot, &error);
        if (!*lock)
        {
            diagnostics.error("failed to lock workspace: " + error);
            return {};
        }
        return lock;
    }
}
