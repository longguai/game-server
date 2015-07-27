#ifndef _DEBUG_CONFIG_H_
#define _DEBUG_CONFIG_H_

#ifndef LOG_LEVEL
#define LOG_LEVEL 4
#endif

#if (!defined LOG_LEVEL) || (LOG_LEVEL == 0)

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) do { } while (0)
#   define LOG_WARN(fmt, ...) do { } while (0)
#   define LOG_ERROR(fmt, ...) do { } while (0)
#   define LOG_ASSERT(expression) do { } while (0)

#elif (LOG_LEVEL == 1)

#   include <crtdbg.h>
#   include "LogUtil.h"

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) do { } while (0)
#   define LOG_WARN(fmt, ...) do { } while (0)
#   define LOG_ERROR(fmt, ...) jw::LogError(fmt, ##__VA_ARGS__)
#   define LOG_ASSERT _ASSERTE

#elif (LOG_LEVEL == 2)

#   include <crtdbg.h>
#   include "LogUtil.h"

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) do { } while (0)
#   define LOG_WARN(fmt, ...) jw::LogWarn(fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) jw::LogError(fmt, ##__VA_ARGS__)
#   define LOG_ASSERT _ASSERTE

#elif (LOG_LEVEL == 3)

#   include <crtdbg.h>
#   include "LogUtil.h"

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) do { } while (0)
#   define LOG_INFO(fmt, ...) jw::LogInfo(fmt, ##__VA_ARGS__)
#   define LOG_WARN(fmt, ...) jw::LogWarn(fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) jw::LogError(fmt, ##__VA_ARGS__)
#   define LOG_ASSERT _ASSERTE

#elif (LOG_LEVEL == 4)

#   include <crtdbg.h>
#   include "LogUtil.h"

#   define LOG_VERBOSE(fmt, ...) do { } while (0)
#   define LOG_DEBUG(fmt, ...) jw::LogDebug(fmt, ##__VA_ARGS__)
#   define LOG_INFO(fmt, ...) jw::LogInfo(fmt, ##__VA_ARGS__)
#   define LOG_WARN(fmt, ...) jw::LogWarn(fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) jw::LogError(fmt, ##__VA_ARGS__)
#   define LOG_ASSERT _ASSERTE

#else

#   include <crtdbg.h>
#   include "LogUtil.h"

#   define LOG_VERBOSE(fmt, ...) jw::LogVerbose(fmt, ##__VA_ARGS__)
#   define LOG_DEBUG(fmt, ...) jw::LogDebug(fmt, ##__VA_ARGS__)
#   define LOG_INFO(fmt, ...) jw::LogInfo(fmt, ##__VA_ARGS__)
#   define LOG_WARN(fmt, ...) jw::LogWarn(fmt, ##__VA_ARGS__)
#   define LOG_ERROR(fmt, ...) jw::LogError(fmt, ##__VA_ARGS__)
#   define LOG_ASSERT _ASSERTE

#endif

#endif
