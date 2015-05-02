#ifndef _IOCP_DEBUG_H_
#define _IOCP_DEBUG_H_

#include <windows.h>
#include <stdio.h>

namespace iocp {
    static void printfToConsole(const char *tag, const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        printf("[%s] ", tag);
        vprintf(fmt, args);
        va_end(args);
    }

    static void printfToWindow(const char *tag, char *fmt, ...) {
        char buf[1024];
        int n = _snprintf(buf, 1024, "[%s] ", tag);
        va_list args;
        va_start(args, fmt);
        int ret = _vsnprintf_s(buf + n, 1024 - n, 1024 - n, fmt, args);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
        va_end(args);
    }

    static void printfToConsoleAndWindow(const char *tag, const char *fmt, ...) {
        char buf[1024];
        int n = _snprintf(buf, 1024, "[%s] ", tag);
        va_list args;
        va_start(args, fmt);
        int ret = _vsnprintf_s(buf + n, 1024 - n, 1024 - n, fmt, args);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
        puts(buf);
        va_end(args);
    }
}

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif

#if (!defined DEBUG_LEVEL) || (DEBUG_LEVEL == 0)

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) do { } while (0)
#   define LOG_WARN(fmt, ...) do { } while (0)
#   define LOG_ERROR(fmt, ...) do { } while (0)
#   define LOG_ASSERT(expression) do { } while (0)

#elif DEBUG_LEVEL == 4

#   include <assert.h>

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) do { } while (0)
#   define LOG_WARN(fmt, ...) do { } while (0)
#   define LOG_ERROR(fmt, ...) iocp::printfToWindow("ERROR", fmt, ##__VA_ARGS__)
#   define LOG_ASSERT(expression) assert(expression)

#elif DEBUG_LEVEL == 2

#   include <assert.h>

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) do { } while (0)
#   define LOG_WARN(fmt, ...) iocp::printfToWindow("WARN", fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) iocp::printfToConsoleAndWindow("ERROR", fmt, ##__VA_ARGS__)
#   define LOG_ASSERT(expression) assert(expression)

#elif DEBUG_LEVEL == 3

#   include <assert.h>

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) iocp::printfToWindow("INFO", fmt, ##__VA_ARGS__)
#   define LOG_WARN(fmt, ...) iocp::printfToConsoleAndWindow("WARN", fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) iocp::printfToConsoleAndWindow("ERROR", fmt, ##__VA_ARGS__)
#   define LOG_ASSERT(expression) assert(expression)

#elif DEBUG_LEVEL == 4

#   include <assert.h>

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) iocp::printfToConsoleAndWindow("DEBUG", fmt, ##__VA_ARGS__)
#   define LOG_INFO(fmt, ...) iocp::printfToWindow("INFO", fmt, ##__VA_ARGS__)
#   define LOG_WARN(fmt, ...) iocp::printfToConsoleAndWindow("WARN", fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) iocp::printfToConsoleAndWindow("ERROR", fmt, ##__VA_ARGS__)
#   define LOG_ASSERT(expression) assert(expression)

#else

#   include <assert.h>

#   define LOG_VERBOSE(fmt, ...) iocp::printfToConsole("VERBOSE", fmt, ##__VA_ARGS__)
#   define LOG_DEBUG(fmt, ...) iocp::printfToConsoleAndWindow("DEBUG", fmt, ##__VA_ARGS__)
#   define LOG_INFO(fmt, ...) iocp::printfToWindow("INFO", fmt, ##__VA_ARGS__)
#   define LOG_WARN(fmt, ...) iocp::printfToConsoleAndWindow("WARN", fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) iocp::printfToConsoleAndWindow("ERROR", fmt, ##__VA_ARGS__)
#   define LOG_ASSERT(expression) assert(expression)

#endif

#endif
