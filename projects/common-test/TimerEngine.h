#ifndef _TIMER_ENGINE_H_
#define _TIMER_ENGINE_H_

#include <stddef.h>
#include <stdint.h>
#include <chrono>
#include <vector>
#include <thread>
#include "QuickMutex.h"

namespace jw {
    struct TimerItem;

    class TimerEngine {
    public:
        TimerEngine();
        ~TimerEngine();

        static const uint32_t REPEAT_FOREVER = UINT32_MAX;

        uintptr_t setTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, const std::function<void (int64_t)> &callback);
        uintptr_t setTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, std::function<void (int64_t)> &&callback);

        bool killTimer(uintptr_t timerId);

    private:
        template <class _Function>
        uintptr_t _SetTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, _Function &&callback);

    private:
        std::vector<TimerItem> _itemsActive;

        QuickMutex _mutex;
        std::thread *_thread;
        volatile bool _shouldQuit;

        void _ThreadProc();
    };
}

#endif
