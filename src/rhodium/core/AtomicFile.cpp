#include "rhodium/core/AtomicFile.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <system_error>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace rhodium
{
    namespace
    {
#ifdef _WIN32
        std::wstring nativePath(const std::filesystem::path &path)
        {
            auto value = std::filesystem::absolute(path).lexically_normal().native();
            if (value.rfind(L"\\\\?\\", 0) == 0)
            {
                return value;
            }
            return value.rfind(L"\\\\", 0) == 0 ? L"\\\\?\\UNC\\" + value.substr(2) : L"\\\\?\\" + value;
        }
#endif
        struct PendingFile
        {
            std::filesystem::path path;
            bool owned = false;
#ifdef _WIN32
            HANDLE handle = INVALID_HANDLE_VALUE;
#else
            int handle = -1;
#endif
            ~PendingFile()
            {
#ifdef _WIN32
                if (handle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(handle);
                }
#else
                if (handle != -1)
                {
                    close(handle);
                }
#endif
                if (owned)
                {
                    std::error_code ignored;
                    std::filesystem::remove(path, ignored);
                }
            }
        };

        std::filesystem::path writeDestination(std::filesystem::path path)
        {
            if (path.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos)
            {
                throw std::invalid_argument("write destination contains a NUL character");
            }
            path = std::filesystem::absolute(path);
            // Retain the established behavior of writing through config-file symlinks.
            for (unsigned hops = 0; hops < 32; ++hops)
            {
                std::error_code ec;
                const bool symlink = std::filesystem::is_symlink(path, ec);
                if (ec && ec != std::errc::no_such_file_or_directory)
                {
                    throw std::system_error(ec, "inspect write destination");
                }
                if (!symlink)
                {
                    return path;
                }
                const auto target = std::filesystem::read_symlink(path);
                path = target.is_absolute() ? target : path.parent_path() / target;
            }
            throw std::system_error(std::make_error_code(std::errc::too_many_symbolic_link_levels));
        }
    }

    bool AtomicFile::write(const std::filesystem::path &path, const std::string &content, std::string *error)
    {
        try
        {
            const auto destination = writeDestination(path);
            PendingFile pending;
            static std::atomic<unsigned long long> sequence{0};
#ifdef _WIN32
            const auto process = GetCurrentProcessId();
            std::vector<unsigned char> securityDescriptor;
            SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE};
            const auto attributes = GetFileAttributesW(nativePath(destination).c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES)
            {
                if (attributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    throw std::system_error(ERROR_DIRECTORY, std::system_category(), "write destination is a directory");
                }
                DWORD size = 0;
                GetFileSecurityW(nativePath(destination).c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &size);
                if (!size)
                {
                    throw std::system_error(GetLastError(), std::system_category(), "inspect destination permissions");
                }
                securityDescriptor.resize(size);
                if (!GetFileSecurityW(nativePath(destination).c_str(), DACL_SECURITY_INFORMATION,
                                      securityDescriptor.data(), size, &size))
                {
                    throw std::system_error(GetLastError(), std::system_category(), "read destination permissions");
                }
                security.lpSecurityDescriptor = securityDescriptor.data();
            }
            else if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                throw std::system_error(GetLastError(), std::system_category(), "inspect write destination");
            }
#else
            const auto process = getpid();
#endif
            for (unsigned attempt = 0; attempt < 32; ++attempt)
            {
                pending.path = destination;
                pending.path += ".tmp." + std::to_string(process) + "." + std::to_string(sequence.fetch_add(1));
#ifdef _WIN32
                pending.handle = CreateFileW(nativePath(pending.path).c_str(), GENERIC_WRITE, 0, &security, CREATE_NEW,
                                             FILE_ATTRIBUTE_NORMAL, nullptr);
                if (pending.handle != INVALID_HANDLE_VALUE)
                {
                    pending.owned = true;
                    break;
                }
                const auto code = GetLastError();
                if (code != ERROR_FILE_EXISTS && code != ERROR_ALREADY_EXISTS)
                {
                    throw std::system_error(code, std::system_category(), "create temporary file");
                }
#else
                pending.handle = open(pending.path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
                if (pending.handle != -1)
                {
                    pending.owned = true;
                    break;
                }
                if (errno != EEXIST)
                {
                    throw std::system_error(errno, std::generic_category(), "create temporary file");
                }
#endif
            }
            if (!pending.owned)
            {
                throw std::system_error(std::make_error_code(std::errc::file_exists), "allocate temporary file");
            }
#ifndef _WIN32
            struct stat temporary;
            if (fstat(pending.handle, &temporary) != 0 || fchmod(pending.handle, 0600) != 0)
            {
                throw std::system_error(errno, std::generic_category(), "protect temporary file");
            }
            auto permissions = temporary.st_mode & 0777;
#endif
            std::size_t offset = 0;
            while (offset < content.size())
            {
#ifdef _WIN32
                DWORD written = 0;
                const auto amount = static_cast<DWORD>(std::min<std::size_t>(content.size() - offset, 1024 * 1024));
                const bool ok = WriteFile(pending.handle, content.data() + offset, amount, &written, nullptr) != FALSE;
                if (!ok || written == 0)
                {
                    throw std::system_error(ok ? ERROR_WRITE_FAULT : GetLastError(), std::system_category(), "write temporary file");
                }
#else
                const auto amount = std::min<std::size_t>(content.size() - offset, 1024 * 1024);
                const auto written = ::write(pending.handle, content.data() + offset, amount);
                if (written < 0 && errno == EINTR)
                {
                    continue;
                }
                if (written <= 0)
                {
                    throw std::system_error(written < 0 ? errno : EIO, std::generic_category(), "write temporary file");
                }
#endif
                offset += static_cast<std::size_t>(written);
            }
#ifdef _WIN32
            if (!FlushFileBuffers(pending.handle))
            {
                throw std::system_error(GetLastError(), std::system_category(), "flush temporary file");
            }
            const bool closed = CloseHandle(pending.handle) != FALSE;
            pending.handle = INVALID_HANDLE_VALUE;
            if (!closed)
            {
                throw std::system_error(GetLastError(), std::system_category(), "close temporary file");
            }
            // ReplaceFile preserves ACLs/attributes of an existing configuration file.
            if (!ReplaceFileW(nativePath(destination).c_str(), nativePath(pending.path).c_str(), nullptr, 0, nullptr, nullptr))
            {
                const auto code = GetLastError();
                if (code != ERROR_FILE_NOT_FOUND ||
                    !MoveFileExW(nativePath(pending.path).c_str(), nativePath(destination).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                {
                    throw std::system_error(GetLastError(), std::system_category(), "replace destination file");
                }
            }
#else
            struct stat original;
            if (stat(destination.c_str(), &original) == 0)
            {
                if (!S_ISREG(original.st_mode))
                {
                    throw std::system_error(EISDIR, std::generic_category(), "write destination is not a regular file");
                }
                permissions = original.st_mode & 0777;
            }
            else if (errno != ENOENT)
            {
                throw std::system_error(errno, std::generic_category(), "inspect destination file");
            }
            if (fchmod(pending.handle, permissions) != 0)
            {
                throw std::system_error(errno, std::generic_category(), "preserve destination permissions");
            }
            if (fsync(pending.handle) != 0)
            {
                throw std::system_error(errno, std::generic_category(), "flush temporary file");
            }
            const auto closed = close(pending.handle);
            pending.handle = -1;
            if (closed != 0)
            {
                throw std::system_error(errno, std::generic_category(), "close temporary file");
            }
            std::filesystem::rename(pending.path, destination);
#endif
            pending.owned = false;
#ifndef _WIN32
            const auto parent = destination.parent_path().empty() ? std::filesystem::path(".") : destination.parent_path();
            const auto directory = open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
            if (directory == -1)
            {
                throw std::system_error(errno, std::generic_category(), "open destination directory");
            }
            const auto synced = fsync(directory);
            const auto code = errno;
            close(directory);
            if (synced != 0 && code != EINVAL && code != ENOTSUP)
            {
                throw std::system_error(code, std::generic_category(), "flush destination directory");
            }
#endif
            return true;
        }
        catch (const std::exception &ex)
        {
            if (error)
            {
                *error = ex.what();
            }
            return false;
        }
    }
}
