/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include <filesystem>
#include <string>

namespace CLI
{
    class App;
}

namespace rhodium
{
    struct CommandContext;

    class CmdLine
    {
    public:
        static int run(int argc, char **argv);
        static std::string version();

    private:
        static void registerDebug(CLI::App &app, CommandContext &context);
        static void registerDependencies(CLI::App &app, CommandContext &context);
        static void registerPackages(CLI::App &app, CommandContext &context);
        static void registerClean(CLI::App &app, CommandContext &context);
        static void registerOverrides(CLI::App &app, CommandContext &context);
        static void registerSdk(CLI::App &app, CommandContext &context);
        static void registerSdkAdd(CLI::App &app, CommandContext &context);

        static std::filesystem::path root(const std::string &value);
    };

} // namespace rhodium
