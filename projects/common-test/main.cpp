#include "BasicServer.hpp"
#include "BasicSession.hpp"

namespace echo {
    using jw::Session;
    class ServerProxy {
    public:
        void acceptCallback(asio::ip::tcp::socket &&socket) {
            std::shared_ptr<Session> s = std::make_shared<Session>(std::move(socket),
                std::bind(&ServerProxy::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            s->start();
        }

    private:
        void _sessionCallback(const std::shared_ptr<Session> &s, jw::SessionEvent event, const char *data, size_t length) {
            if (data != nullptr) {
                s->deliver(data, length);
            }
        }
    };

    typedef jw::BasicServer<ServerProxy, 128> Server;
}

#include <unordered_set>

namespace chat {
    using jw::Session;
    class ServerProxy {
    public:
        void acceptCallback(asio::ip::tcp::socket &&socket) {
            std::shared_ptr<Session> s = std::make_shared<Session>(std::move(socket),
                std::bind(&ServerProxy::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            _mutex.lock();
            _sessions.insert(s);
            _mutex.unlock();
            s->start();
        }

    private:
        void _sessionCallback(const std::shared_ptr<Session> &s, jw::SessionEvent event, const char *data, size_t length) {
            if (data != nullptr) {
                _mutex.lock();
                std::for_each(_sessions.begin(), _sessions.end(), [data, length](const std::shared_ptr<Session> &s) {
                    s->deliver(data, length);
                });
                _mutex.unlock();
            }
            else {
                _mutex.lock();
                _sessions.erase(s);
                _mutex.unlock();
            }
        }

    private:
        std::unordered_set<std::shared_ptr<Session> > _sessions;
        std::mutex _mutex;
    };

    typedef jw::BasicServer<ServerProxy, 128> Server;
}

#include "DebugConfig.h"
#include <thread>
#include <chrono>
#include <stdio.h>

#include "TimerEngine.h"

#include "IOService.hpp"

int main() {
    jw::IOService<chat::Server> s(8899);
    (void)s;
    getchar();
    return 0;

    jw::TimerEngine te;
    te.registerTimer(1, std::chrono::milliseconds(1000), jw::TimerEngine::REPEAT_FOREVER, [&te](int64_t dt) {
        LOG_INFO("timer dt = %I64d", dt);
        te.unregisterTimer(1);
        te.registerTimer(2, std::chrono::milliseconds(500), 20, [](int64_t dt) { LOG_INFO("dt = %I64d", dt); });
    });
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
