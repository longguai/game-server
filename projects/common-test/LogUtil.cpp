#include "LogUtil.h"
#include <windows.h>
#include <crtdbg.h>
#include <stdio.h>
#include <time.h>
#include "QuickMutex.h"

namespace jw {
    QuickMutex g_lock;

    static inline void _LogWithTag(const char *tag, WORD attributes, const char *format, va_list args) {
        std::lock_guard<QuickMutex> g(g_lock);
        (void)g;

        char buf[1024];
        time_t now = time(nullptr);
        struct tm t;
        errno_t err = localtime_s(&t, &now);
        size_t offset = 0;
        if (err == 0) {
            offset = strftime(buf, 1024, "[%Y-%m-%d %H:%M:%S", &t);
            offset += sprintf_s(buf + offset, 1024 - offset, " %s] ", tag);
        }
        else {
            offset = sprintf_s(buf, 1024, "[%s %s] ", "0000-00-00 00:00:00", tag);
        }

        _vsnprintf_s(buf + offset, 1024 - offset, 1024 - offset, format, args);
        HANDLE hConsoleOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsoleOutput != INVALID_HANDLE_VALUE) {
            ::SetConsoleTextAttribute(hConsoleOutput, attributes);
            puts(buf);
            ::SetConsoleTextAttribute(hConsoleOutput, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else {
            puts(buf);
        }
        fflush(stdout);
        ::OutputDebugStringA(buf);
        ::OutputDebugStringA("\n");
    }

    void LogVerbose(const char *format, ...) {
        va_list args;
        va_start(args, format);
        _LogWithTag("VERBOSE", FOREGROUND_BLUE | FOREGROUND_INTENSITY, format, args);
        va_end(args);
    }

    void LogDebug(const char *format, ...) {
        va_list args;
        va_start(args, format);
        _LogWithTag("DEBUG", FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, format, args);
        va_end(args);
    }

    void LogInfo(const char *format, ...) {
        va_list args;
        va_start(args, format);
        _LogWithTag("INFO", FOREGROUND_GREEN | FOREGROUND_INTENSITY, format, args);
        va_end(args);
    }

    void LogWarn(const char *format, ...) {
        va_list args;
        va_start(args, format);
        _LogWithTag("WARN", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, format, args);
        va_end(args);
    }

    void LogError(const char *format, ...) {
        va_list args;
        va_start(args, format);
        _LogWithTag("ERROR", FOREGROUND_RED | FOREGROUND_INTENSITY, format, args);
        va_end(args);
    }
}
