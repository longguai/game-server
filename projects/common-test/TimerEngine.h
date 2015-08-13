#ifndef _TIMER_ENGINE_H_
#define _TIMER_ENGINE_H_

#include <stddef.h>
#include <stdint.h>
#include <chrono>
#include <vector>
#include <thread>
#include <memory>
#include "QuickMutex.h"

namespace jw {
    struct TimerItem;

    class TimerEngine {
    public:
        static TimerEngine *getInstance();
        static void destroyInstance();

        static const uint32_t REPEAT_FOREVER = UINT32_MAX;

        /**
         * @param timerId ID （必须全局唯一）
         * @param elapse 间隔
         * @param repeatTimes 重复次数
         * @param callback 回调函数
         */
        uintptr_t registerTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, const std::function<void (int64_t)> &callback);
        uintptr_t registerTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, std::function<void (int64_t)> &&callback);

        bool unregisterTimer(uintptr_t timerId);

    private:
        TimerEngine();
        ~TimerEngine();

        template <class _Function>
        uintptr_t _RegisterTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, _Function &&callback);

    private:
        std::vector<std::shared_ptr<TimerItem> > _itemsActive;
        QuickMutex _mutex;

        std::thread *_thread = nullptr;
        volatile bool _shouldQuit = false;

        void _ThreadProc();
    };
}

#endif
