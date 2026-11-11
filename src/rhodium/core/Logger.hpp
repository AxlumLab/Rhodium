/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include <string_view>
#include <string>
#include <utility>

#include <fmt/format.h>

namespace rhodium
{
#define BUILD_DEBUG 1

	enum class LogLevel
	{
		kWarning,
		kError,
		kDebug,
		kInfo,
		kTrace,
		kSuccess
	};

	class Logger final
	{
	private:
		void emitLog(const std::string &msg, LogLevel level);

		template <typename... Args>
		inline void emitFormatted(LogLevel level, std::string_view fmtStr, Args &&...args)
		{
			emitLog(fmt::format(fmt::runtime(fmtStr), std::forward<Args>(args)...), level);
		}

	public:
		Logger(const char *name, bool record);
		~Logger() = default;

		template <typename... Args>
		inline void info(std::string_view fmtStr, Args &&...args)
		{
			emitFormatted(LogLevel::kInfo, fmtStr, std::forward<Args>(args)...);
		}
		inline void info(std::string_view msg)
		{
			emitLog(std::string(msg), LogLevel::kInfo);
		}

		template <typename... Args>
		inline void warning(std::string_view fmtStr, Args &&...args)
		{
			emitFormatted(LogLevel::kWarning, fmtStr, std::forward<Args>(args)...);
		}
		inline void warning(std::string_view msg)
		{
			emitLog(std::string(msg), LogLevel::kWarning);
		}

		template <typename... Args>
		inline void error(std::string_view fmtStr, Args &&...args)
		{
			emitFormatted(LogLevel::kError, fmtStr, std::forward<Args>(args)...);
		}
		inline void error(std::string_view msg)
		{
			emitLog(std::string(msg), LogLevel::kError);
		}

		template <typename... Args>
		inline void success(std::string_view fmtStr, Args &&...args)
		{
			emitFormatted(LogLevel::kSuccess, fmtStr, std::forward<Args>(args)...);
		}
		inline void success(std::string_view msg)
		{
			emitLog(std::string(msg), LogLevel::kSuccess);
		}

		template <typename... Args>
		inline void debug(std::string_view fmtStr, Args &&...args)
		{
#if BUILD_DEBUG
			emitFormatted(LogLevel::kDebug, fmtStr, std::forward<Args>(args)...);
#else
			(void)fmtStr;
			((void)args, ...);
#endif
		}
		inline void debug(std::string_view msg)
		{
#if BUILD_DEBUG
			emitLog(std::string(msg), LogLevel::kDebug);
#else
			(void)msg;
#endif
		}

		template <typename... Args>
		inline void trace(std::string_view fmtStr, Args &&...args)
		{
#if BUILD_DEBUG
			emitFormatted(LogLevel::kTrace, fmtStr, std::forward<Args>(args)...);
#else
			(void)fmtStr;
			((void)args, ...);
#endif
		}
		inline void trace(std::string_view msg)
		{
#if BUILD_DEBUG
			emitLog(std::string(msg), LogLevel::kTrace);
#else
			(void)msg;
#endif
		}
	};

	Logger *getDefaultLogger(const char *name = "", bool useFileRecorder = false);
}

#define LogInfo(...) ::rhodium::getDefaultLogger()->info(__VA_ARGS__)
#define LogWarn(...) ::rhodium::getDefaultLogger()->warning(__VA_ARGS__)
#define LogError(...) ::rhodium::getDefaultLogger()->error(__VA_ARGS__)
#define LogSuccess(...) ::rhodium::getDefaultLogger()->success(__VA_ARGS__)
#define LogDebug(...) ::rhodium::getDefaultLogger()->debug(__VA_ARGS__)
#define LogTrace(...) ::rhodium::getDefaultLogger()->trace(__VA_ARGS__)
