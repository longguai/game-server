#include "TimerEngine.h"
#include "DebugConfig.h"
#include <algorithm>

namespace jw {
    struct TimerItem {
        uintptr_t timerId;
        std::chrono::milliseconds elapse;
        std::chrono::high_resolution_clock::time_point lastTime;
        std::chrono::high_resolution_clock::time_point nextTime;
        uint32_t repeatTimes;
        std::function<void (int64_t)> callback;
    };

    TimerEngine::TimerEngine() {
        _shouldQuit = false;
        _thread = new std::thread(std::bind(&TimerEngine::_ThreadProc, this));
    }

    TimerEngine::~TimerEngine() {
        if (_thread != nullptr) {
            _shouldQuit = true;
            if (_thread->joinable()) {
                _thread->join();
            }
            delete _thread;
            _thread = nullptr;
        }
    }

    template <class _Function>
    uintptr_t TimerEngine::_SetTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, _Function &&callback) {
        LOG_ASSERT(timerId != 0);
        if (timerId == 0) {
            return 0;
        }

        std::lock_guard<QuickMutex> g(_mutex);
        (void)g;

        std::vector<TimerItem>::iterator it = std::find_if(_itemsActive.begin(), _itemsActive.end(), [timerId](const TimerItem &item) {
            return item.timerId == timerId;
        });
        TimerItem *pTimerItem;
        if (it == _itemsActive.end()) {
            try {
                _itemsActive.push_back(TimerItem());
            }
            catch (...) {
                return 0;
            }
            pTimerItem = &_itemsActive.back();
        }
        else {
            pTimerItem = &*it;
        }

        pTimerItem->timerId = timerId;
        pTimerItem->elapse = elapse;
        pTimerItem->repeatTimes = repeatTimes;
        pTimerItem->callback = callback;

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        pTimerItem->lastTime = now;
        pTimerItem->nextTime = now + std::chrono::milliseconds(pTimerItem->elapse);
        return timerId;
    }

    uintptr_t TimerEngine::setTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, const std::function<void (int64_t)> &callback) {
        return _SetTimer(timerId, elapse, repeatTimes, callback);
    }

    uintptr_t TimerEngine::setTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, std::function<void (int64_t)> &&callback) {
        return _SetTimer(timerId, elapse, repeatTimes, callback);
    }

    bool TimerEngine::killTimer(uintptr_t timerId) {
        std::lock_guard<QuickMutex> g(_mutex);
        (void)g;

        std::vector<TimerItem>::iterator it = std::find_if(_itemsActive.begin(), _itemsActive.end(), [timerId](const TimerItem &item) {
            return item.timerId == timerId;
        });
        if (it == _itemsActive.end()) {
            return false;
        }
        _itemsActive.erase(it);
        return true;
    }

    void TimerEngine::_ThreadProc() {
        std::chrono::high_resolution_clock::time_point prev = std::chrono::high_resolution_clock::now();

        while (!_shouldQuit) {
            {
                std::lock_guard<QuickMutex> g(_mutex);
                (void)g;

                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

                for (std::vector<TimerItem>::iterator it = _itemsActive.begin(); it != _itemsActive.end();) {
                    TimerItem *pTimerItem = &*it;
                    if (pTimerItem->nextTime > now) {
                        ++it;
                        continue;
                    }

                    bool erased = false;
                    int64_t dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - pTimerItem->lastTime).count();
                    try {
                        pTimerItem->callback(dt);
                    }
                    catch (std::exception &e) {
                        LOG_ERROR("%s", e.what());
                    }

                    if (pTimerItem->repeatTimes != REPEAT_FOREVER) {
                        --pTimerItem->repeatTimes;
                        if (pTimerItem->repeatTimes == 0) {
                            erased = true;
                            it = _itemsActive.erase(it);
                        }
                    }

                    if (!erased) {
                        pTimerItem->lastTime = now;
                        pTimerItem->nextTime += std::chrono::milliseconds(pTimerItem->elapse);
                        ++it;
                    }
                }
                prev = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
