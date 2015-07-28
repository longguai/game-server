#include "DebugConfig.h"
#include <thread>
#include <chrono>
#include <stdio.h>

#include "TimerEngine.h"

int main() {
    jw::TimerEngine te;
    te.setTimer(1, std::chrono::milliseconds(1000), jw::TimerEngine::REPEAT_FOREVER, [](int64_t dt){ LOG_INFO("timer dt = %I64d", dt); });
    getchar();

    return 0;

#define ESCAPE std::chrono::seconds(1)
    puts("begin");

    //freopen("E:/GitHub/server-test/projects/json-test/log.txt", "w", stdout);

    std::thread t1([] {
        while (1) {
            LOG_VERBOSE("verbose");
            std::this_thread::sleep_for(ESCAPE);
        }
    });

    puts("verbose");
    std::thread t2([] {
        while (2) {
            LOG_DEBUG("debug");
            std::this_thread::sleep_for(ESCAPE);
        }
    });
    puts("debug");
    std::thread t3([] {
        while (1) {
            LOG_INFO("info");
            std::this_thread::sleep_for(ESCAPE);
        }
    });
    puts("info");
    std::thread t4([] {
        while (1) {
            LOG_WARN("warn");
            std::this_thread::sleep_for(ESCAPE);
        }
    });
    puts("warn");
    std::thread t5([] {
        while (1) {
            LOG_ERROR("error");
            std::this_thread::sleep_for(ESCAPE);
        }
    });
    puts("error");
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    return 0;
}
