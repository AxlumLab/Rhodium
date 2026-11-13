/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "Logger.hpp"

#include <memory>
#include <string>
#include <vector>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/spdlog.h>

// For info level, don't use color markers
#define INFO_FORMAT "info: %v"
#define SUCCESS_FORMAT "success: %v"
#define SUCCESS_FORMAT_COLORED "\033[32msuccess:\033[0m %v"
#define COLORED_FORMAT "%^%l:%$ %v"
#define FILE_FORMAT "%l: %v"

namespace rhodium
{

    static std::unique_ptr<spdlog::logger> s_logger;
    static std::unique_ptr<spdlog::logger> s_info_logger;
    static std::unique_ptr<spdlog::logger> s_success_logger;

    void Logger::emitLog(const std::string &msg, LogLevel level)
    {
        if (!s_logger)
        {
            Logger("", false);
        }
        switch (level)
        {
        case LogLevel::kWarning:
            s_logger->warn("{}", msg);
            break;
        case LogLevel::kError:
            s_logger->error("{}", msg);
            break;
        case LogLevel::kDebug:
            s_logger->debug("{}", msg);
            break;
        case LogLevel::kInfo:
            // Use separate logger for info to avoid colors
            s_info_logger->info("{}", msg);
            break;
        case LogLevel::kSuccess:
            // Use custom logger for success with green color (mapped to info level)
            s_success_logger->info("{}", msg);
            break;
        case LogLevel::kTrace:
            s_logger->trace("{}", msg);
            break;
        }
    }

    Logger::Logger(const char *name, bool record)
    {
        auto fileName = std::string(name) + ".log";

        if (s_logger)
        {
            return;
        }

        std::vector<spdlog::sink_ptr> logSinks;
        std::vector<spdlog::sink_ptr> infoLogSinks;
        std::vector<spdlog::sink_ptr> successLogSinks;

        // Console sink with colors for warn/error/debug
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(COLORED_FORMAT);
        logSinks.emplace_back(console_sink);

        // Console sink without colors for info
        auto info_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        info_console_sink->set_pattern(INFO_FORMAT);
        infoLogSinks.emplace_back(info_console_sink);

        // Console sink with green color for success
        auto success_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        success_console_sink->set_pattern(SUCCESS_FORMAT_COLORED);
        successLogSinks.emplace_back(success_console_sink);

        // File sink without colors
        if (record)
        {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fileName, true);
            file_sink->set_pattern(FILE_FORMAT);
            logSinks.emplace_back(file_sink);
            infoLogSinks.emplace_back(file_sink);
            successLogSinks.emplace_back(file_sink);
        }

        s_logger = std::make_unique<spdlog::logger>(name, begin(logSinks), end(logSinks));
        s_logger->set_level(spdlog::level::trace);
        s_logger->flush_on(spdlog::level::trace);

        s_info_logger =
            std::make_unique<spdlog::logger>(std::string(name) + "_info", begin(infoLogSinks), end(infoLogSinks));
        s_info_logger->set_level(spdlog::level::trace);
        s_info_logger->flush_on(spdlog::level::trace);

        s_success_logger = std::make_unique<spdlog::logger>(std::string(name) + "_success", begin(successLogSinks),
                                                            end(successLogSinks));
        s_success_logger->set_level(spdlog::level::info);
        s_success_logger->flush_on(spdlog::level::info);
    }

    Logger *Logger::get(const char *name, bool useFileRecorder)
    {
        static Logger logger{name, useFileRecorder};
        return &logger;
    }

} // namespace rhodium
