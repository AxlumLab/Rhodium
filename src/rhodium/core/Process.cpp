/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#include "rhodium/core/Process.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace rhodium
{
    namespace
    {
        class NativeProcess final
        {
        public:
#ifdef _WIN32
            static bool needsQuote(const std::string &value)
            {
                return value.empty() || value.find_first_of(" \t\n\v\"") != std::string::npos;
            }

            static std::string quote(const std::string &value)
            {
                if (!needsQuote(value))
                {
                    return value;
                }

                std::string result = "\"";
                std::size_t backslashes = 0;
                for (char c : value)
                {
                    if (c == '\\')
                    {
                        ++backslashes;
                        continue;
                    }
                    if (c == '"')
                    {
                        result.append(backslashes * 2 + 1, '\\');
                        result.push_back('"');
                        backslashes = 0;
                        continue;
                    }
                    result.append(backslashes, '\\');
                    backslashes = 0;
                    result.push_back(c);
                }
                result.append(backslashes * 2, '\\');
                result.push_back('"');
                return result;
            }

            static std::string commandLine(const std::string &executable, const std::vector<std::string> &args)
            {
                std::string command = quote(executable);
                for (const auto &arg : args)
                {
                    command.push_back(' ');
                    command += quote(arg);
                }
                return command;
            }

            static std::map<std::string, std::string> environment()
            {
                std::map<std::string, std::string> result;
                LPCH block = GetEnvironmentStringsA();
                if (!block)
                {
                    return result;
                }
                for (LPCH item = block; *item != '\0'; item += std::strlen(item) + 1)
                {
                    std::string entry(item);
                    const auto equals = entry.find('=');
                    if (equals == std::string::npos || equals == 0)
                    {
                        continue;
                    }
                    result[entry.substr(0, equals)] = entry.substr(equals + 1);
                }
                FreeEnvironmentStringsA(block);
                return result;
            }

            static std::vector<char> environmentBlock(const std::map<std::string, std::string> &environment)
            {
                if (environment.empty())
                {
                    return {};
                }
                auto merged = NativeProcess::environment();
                for (const auto &[key, value] : environment)
                {
                    auto existing = std::find_if(merged.begin(), merged.end(),
                                                 [&](const auto &item)
                                                 {
                                                     if (item.first.size() != key.size())
                                                     {
                                                         return false;
                                                     }
                                                     for (std::size_t i = 0; i < key.size(); ++i)
                                                     {
                                                         if (std::tolower(static_cast<unsigned char>(item.first[i])) !=
                                                             std::tolower(static_cast<unsigned char>(key[i])))
                                                         {
                                                             return false;
                                                         }
                                                     }
                                                     return true;
                                                 });
                    if (existing != merged.end())
                    {
                        merged.erase(existing);
                    }
                    merged[key] = value;
                }

                std::vector<char> block;
                for (const auto &[key, value] : merged)
                {
                    const auto entry = key + "=" + value;
                    block.insert(block.end(), entry.begin(), entry.end());
                    block.push_back('\0');
                }
                block.push_back('\0');
                return block;
            }

            static ProcessResult runWindows(const std::string &commandLine,
                                            const std::map<std::string, std::string> &environment = {})
            {
                ProcessResult result;

                SECURITY_ATTRIBUTES security{};
                security.nLength = sizeof(security);
                security.bInheritHandle = TRUE;

                HANDLE readPipe = nullptr;
                HANDLE writePipe = nullptr;
                if (!CreatePipe(&readPipe, &writePipe, &security, 0))
                {
                    result.output = "failed to create process pipe";
                    return result;
                }
                SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

                STARTUPINFOA startup{};
                startup.cb = sizeof(startup);
                startup.dwFlags = STARTF_USESTDHANDLES;
                startup.hStdOutput = writePipe;
                startup.hStdError = writePipe;
                startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

                PROCESS_INFORMATION process{};
                std::vector<char> mutableCommand(commandLine.begin(), commandLine.end());
                mutableCommand.push_back('\0');
                auto environmentBlock = NativeProcess::environmentBlock(environment);

                const BOOL started = CreateProcessA(
                    nullptr, mutableCommand.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                    environmentBlock.empty() ? nullptr : environmentBlock.data(), nullptr, &startup, &process);

                CloseHandle(writePipe);

                if (!started)
                {
                    CloseHandle(readPipe);
                    result.output = "failed to start process: " + commandLine;
                    return result;
                }

                std::array<char, 4096> buffer{};
                DWORD bytesRead = 0;
                while (ReadFile(readPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) &&
                       bytesRead > 0)
                {
                    result.output.append(buffer.data(), bytesRead);
                }

                WaitForSingleObject(process.hProcess, INFINITE);
                DWORD exitCode = 1;
                GetExitCodeProcess(process.hProcess, &exitCode);
                result.exitCode = static_cast<int>(exitCode);

                CloseHandle(process.hThread);
                CloseHandle(process.hProcess);
                CloseHandle(readPipe);
                return result;
            }
#endif

            static ProcessResult run(const std::string &executable, const std::vector<std::string> &args,
                                     const std::map<std::string, std::string> &environment = {})
            {
#ifdef _WIN32
                return runWindows(commandLine(executable, args), environment);
#else
                ProcessResult result;
                int pipeFd[2] = {-1, -1};
                if (pipe(pipeFd) != 0)
                {
                    result.output = "failed to create process pipe";
                    return result;
                }

                const pid_t pid = fork();
                if (pid == -1)
                {
                    close(pipeFd[0]);
                    close(pipeFd[1]);
                    result.output = "failed to fork process: " + executable;
                    return result;
                }

                if (pid == 0)
                {
                    for (const auto &[key, value] : environment)
                    {
                        setenv(key.c_str(), value.c_str(), 1);
                    }
                    dup2(pipeFd[1], STDOUT_FILENO);
                    dup2(pipeFd[1], STDERR_FILENO);
                    close(pipeFd[0]);
                    close(pipeFd[1]);

                    std::vector<char *> argv;
                    argv.push_back(const_cast<char *>(executable.c_str()));
                    for (const auto &arg : args)
                    {
                        argv.push_back(const_cast<char *>(arg.c_str()));
                    }
                    argv.push_back(nullptr);
                    execvp(executable.c_str(), argv.data());
                    _exit(127);
                }

                close(pipeFd[1]);
                std::array<char, 4096> buffer{};
                ssize_t bytesRead = 0;
                while ((bytesRead = read(pipeFd[0], buffer.data(), buffer.size())) > 0)
                {
                    result.output.append(buffer.data(), static_cast<std::size_t>(bytesRead));
                }
                close(pipeFd[0]);

                int status = 0;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status))
                {
                    result.exitCode = WEXITSTATUS(status);
                }
                else
                {
                    result.exitCode = status;
                }
                return result;
#endif
            }
        };
    }

    ProcessResult Process::run(const std::string &executable, const std::vector<std::string> &args)
    {
        return NativeProcess::run(executable, args);
    }

    ProcessResult Process::run(const std::string &executable, const std::vector<std::string> &args,
                               const std::map<std::string, std::string> &environment)
    {
        return NativeProcess::run(executable, args, environment);
    }

    ProcessResult Process::runShell(const std::string &command, ProcessShell shell)
    {
        return runShell(command, shell, {});
    }

    ProcessResult Process::runShell(const std::string &command, ProcessShell shell,
                                    const std::map<std::string, std::string> &environment)
    {
#ifdef _WIN32
        if (shell == ProcessShell::Cmd)
        {
            return NativeProcess::runWindows("cmd.exe /D /S /C \"" + command + "\"", environment);
        }
        if (shell == ProcessShell::PowerShell)
        {
            return NativeProcess::runWindows(
                "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"" + command + "\"", environment);
        }
#endif
        switch (shell)
        {
        case ProcessShell::Cmd:
            return NativeProcess::run("cmd.exe", {"/S", "/C", command}, environment);
        case ProcessShell::PowerShell:
            return NativeProcess::run("powershell.exe",
                                      {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command}, environment);
        case ProcessShell::Sh:
            return NativeProcess::run("sh", {"-c", command}, environment);
        case ProcessShell::Direct:
            break;
        }
        return NativeProcess::run(command, {}, environment);
    }

} // namespace rhodium
