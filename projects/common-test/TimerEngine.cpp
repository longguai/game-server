#include "TimerEngine.h"
#include "DebugConfig.h"
#include <algorithm>
#include <inttypes.h>  // for PRIu64

namespace jw {
    struct TimerItem {
        uintptr_t timerId;
        std::chrono::milliseconds elapse;
        std::chrono::high_resolution_clock::time_point lastTime;
        std::chrono::high_resolution_clock::time_point nextTime;
        uint32_t repeatTimes;
        std::function<void (int64_t)> callback;
    };

    static TimerEngine *s_SharedEngine = nullptr;
    TimerEngine *TimerEngine::getInstance() {
        if (s_SharedEngine == nullptr) {
            s_SharedEngine = new (std::nothrow) TimerEngine();
        }
        return s_SharedEngine;
    }

    void TimerEngine::destroyInstance() {
        if (s_SharedEngine != nullptr) {
            delete s_SharedEngine;
            s_SharedEngine = nullptr;
        }
    }

    TimerEngine::TimerEngine() {
        _shouldQuit = false;
        _thread = new std::thread(std::bind(&TimerEngine::_ThreadProc, this));
    }

    TimerEngine::~TimerEngine() {
        _shouldQuit = true;
        if (_thread != nullptr) {
            _thread->join();

            delete _thread;
            _thread = nullptr;
        }
    }

    template <class _Function>
    uintptr_t TimerEngine::_RegisterTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, _Function &&callback) {
        LOG_ASSERT(timerId != 0);
        if (timerId == 0) {
            return 0;
        }

        // 找ID相同的
        std::vector<std::shared_ptr<TimerItem> >::iterator it = std::find_if(_itemsActive.begin(), _itemsActive.end(), [timerId](const std::shared_ptr<TimerItem> &item) {
            return item->timerId == timerId;
        });
        TimerItem *pTimerItem;
        if (it == _itemsActive.end()) {  // 没找到，增加一个新的
            try {
                _itemsActive.push_back(std::make_shared<TimerItem>());
            }
            catch (...) {
                return 0;
            }
            pTimerItem = _itemsActive.back().get();
        }
        else {  // 找到，则修改
            LOG_INFO("Timer ID %" PRIu64 " is already used, it well be updated", (uint64_t)timerId);
            pTimerItem = it->get();
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

    uintptr_t TimerEngine::registerTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, const std::function<void (int64_t)> &callback) {
        std::lock_guard<QuickMutex> g(_mutex);
        (void)g;
        return _RegisterTimer(timerId, elapse, repeatTimes, callback);
    }

    uintptr_t TimerEngine::registerTimer(uintptr_t timerId, std::chrono::milliseconds elapse, uint32_t repeatTimes, std::function<void (int64_t)> &&callback) {
        std::lock_guard<QuickMutex> g(_mutex);
        (void)g;
        return _RegisterTimer(timerId, elapse, repeatTimes, callback);
    }

    bool TimerEngine::unregisterTimer(uintptr_t timerId) {
        std::lock_guard<QuickMutex> g(_mutex);
        (void)g;
        std::vector<std::shared_ptr<TimerItem> >::iterator it = std::find_if(_itemsActive.begin(), _itemsActive.end(), [timerId](const std::shared_ptr<TimerItem> &item) {
            return item->timerId == timerId;
        });
        if (it == _itemsActive.end()) {
            LOG_ERROR("Timer ID %" PRIu64 " is not found", (uint64_t)timerId);
            return false;
        }
        _itemsActive.erase(it);
        return true;
    }

    void TimerEngine::_ThreadProc() {
        while (!_shouldQuit) {
            // 复制一份数组，用这个备份的数组来遍历
            // 否则在回调函数里面调用registerTimer或unregisterTimer会由于争夺_mutex失败造成线程死锁
            std::vector<std::shared_ptr<TimerItem> > itemsActive;
            {
                std::lock_guard<QuickMutex> g(_mutex);
                (void)g;
                itemsActive = _itemsActive;
            }

            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

            // 遍历备份的数组
            std::for_each(itemsActive.begin(), itemsActive.end(), [&now](const std::shared_ptr<TimerItem> &item) {
                TimerItem *pTimerItem = item.get();
                if (pTimerItem->nextTime <= now) {  // 已到下次调用时间的
                    int64_t dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - pTimerItem->lastTime).count();
                    try {
                        pTimerItem->callback(dt);
                    }
                    catch (std::exception &e) {
                        LOG_ERROR("%s", e.what());
                    }

                    if (pTimerItem->repeatTimes != REPEAT_FOREVER) {  // 不是永远重复的，要递减重复次数
                        --pTimerItem->repeatTimes;
                    }

                    if (pTimerItem->repeatTimes > 0) {  // 还有重复次数
                        pTimerItem->lastTime = now;  // 将上次调用时间设置为当前时间
                        pTimerItem->nextTime += std::chrono::milliseconds(pTimerItem->elapse);  // 下次调用时间增加一个时间间隔
                    }
                }
            });

            {
                std::lock_guard<QuickMutex> g(_mutex);
                (void)g;

                // 遍历原数组，删除那些repeatTimes为0的定时器
                for (std::vector<std::shared_ptr<TimerItem> >::iterator it = _itemsActive.begin(); it != _itemsActive.end(); ) {
                    if (it->get()->repeatTimes == 0) {
                        it = _itemsActive.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            //std::this_thread::yield();  // 这种实测比较耗CPU
        }
    }
}
