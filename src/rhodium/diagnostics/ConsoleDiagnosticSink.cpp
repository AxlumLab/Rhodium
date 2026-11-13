/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/diagnostics/ConsoleDiagnosticSink.hpp"

#include "rhodium/core/Logger.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace rhodium
{

    void ConsoleDiagnosticSink::report(const Diagnostic &diagnostic)
    {
        switch (diagnostic.level)
        {
        case DiagnosticLevel::Info:
            LogInfo("{}", diagnostic.message);
            break;
        case DiagnosticLevel::Warning:
            LogWarn("{}", diagnostic.message);
            break;
        case DiagnosticLevel::Error:
            LogError("{}", diagnostic.message);
            break;
        case DiagnosticLevel::Success:
            {
#ifdef _WIN32
                // Use Windows Console API for reliable color on Windows
                HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
                GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
                WORD saved_attributes = consoleInfo.wAttributes;

                // Set green color (FOREGROUND_GREEN)
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                printf("success: %s\n", diagnostic.message.c_str());

                // Restore original color
                SetConsoleTextAttribute(hConsole, saved_attributes);
#else
                // Use ANSI codes on Unix
                printf("\033[32msuccess: %s\033[0m\n", diagnostic.message.c_str());
#endif
            }
            break;
        }
    }

} // namespace rhodium
