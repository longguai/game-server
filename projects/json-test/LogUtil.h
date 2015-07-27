#ifndef _LOG_H_
#define _LOG_H_

#if (!defined __cplusplus)
#   error "You need a C++ compiler for C++ support!"
#endif

namespace jw {
    void LogVerbose(const char *format, ...);
    void LogDebug(const char *format, ...);
    void LogInfo(const char *format, ...);
    void LogWarn(const char *format, ...);
    void LogError(const char *format, ...);
}

#endif