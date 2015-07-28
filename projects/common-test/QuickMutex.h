#ifndef _QUICK_MUTEX_H_
#define _QUICK_MUTEX_H_

#include <mutex>

#if (defined _WIN32) || (defined WIN32)
#include <windows.h>

namespace jw {
    class QuickMutex {
        CRITICAL_SECTION _cs;
    public:
        QuickMutex() { ::InitializeCriticalSection(&_cs); }

        ~QuickMutex() { ::DeleteCriticalSection(&_cs); }

        void lock() { ::EnterCriticalSection(&_cs); }
        void unlock() { ::LeaveCriticalSection(&_cs); }
        bool try_lock() { return !!::TryEnterCriticalSection(&_cs); }

        typedef CRITICAL_SECTION *native_handle_type;
        native_handle_type native_handle() { return &_cs; }

        QuickMutex(const QuickMutex &) = delete;
        QuickMutex &operator=(const QuickMutex &) = delete;

        QuickMutex(QuickMutex &&) = delete;
        QuickMutex &operator=(QuickMutex &&) = delete;
    };
}

#else

namespace jw {
    typedef std::mutex QuickMutex;
}
#endif

#endif
