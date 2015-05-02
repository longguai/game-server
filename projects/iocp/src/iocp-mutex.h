#ifndef _IOCP_MUTEX_H_
#define _IOCP_MUTEX_H_

#include <windows.h>

namespace iocp {
    class mutex final {
    public:
        mutex() { ::InitializeCriticalSection(&_cs); }
        ~mutex() { ::DeleteCriticalSection(&_cs); }

        void lock() { ::EnterCriticalSection(&_cs); }
        bool try_lock() { return ::TryEnterCriticalSection(&_cs) != 0; }
        void unlock() { ::LeaveCriticalSection(&_cs); }

    private:
        CRITICAL_SECTION _cs;
    };
}

#endif
