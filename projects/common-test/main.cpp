#include "BasicServer.hpp"
#include "BasicSession.hpp"
#include "IOService.hpp"
#include "PacketSplitter.hpp"

#include <unordered_set>

#include "DebugConfig.h"
#include <thread>
#include <chrono>
#include <stdio.h>

#include "TimerEngine.h"

namespace gs {
    typedef jw::BasicSession<jw::JsonPacketSplitter, 1024U> Session;

    class ServerProxy {
    public:
        void acceptCallback(asio::ip::tcp::socket &&socket) {
            std::shared_ptr<Session> s = std::make_shared<Session>(std::move(socket),
                std::bind(&ServerProxy::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            {
                std::lock_guard<jw::QuickMutex> g(_mutex);
                (void)g;

                _sessions.insert(s);
            }
            s->start();
        }

    private:
        void _sessionCallback(const std::shared_ptr<Session> &s, jw::SessionEvent event, const char *data, size_t length) {
            if (data != nullptr) {
                try {
                    jw::cppJSON jsonRecv;
                    unsigned cmd, tag;
                    s->decodeRecvPacket(jsonRecv, cmd, tag, data, length);
                    if (cmd == 0) {
                        return;
                    }

                    std::string content = jsonRecv.getValueByKey<std::string>("content");

                    jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
                    jsonSend.insert(std::make_pair("ip", s->getRemoteIP()));
                    jsonSend.insert(std::make_pair("port", s->getRemotePort()));
                    jsonSend.insert(std::make_pair("content", content));

                    std::vector<char> buf = s->encodeSendPacket(cmd, tag, jsonSend);

                    std::lock_guard<jw::QuickMutex> g(_mutex);
                    (void)g;
                    std::for_each(_sessions.begin(), _sessions.end(), [&buf](const std::shared_ptr<Session> &s) {
                        s->deliver(&buf.at(0), buf.size());
                    });
                }
                catch (std::exception &e) {
                    LOG_ERROR("%s", e.what());

                    std::lock_guard<jw::QuickMutex> g(_mutex);
                    (void)g;

                    _sessions.erase(s);
                }
            }
            else {
                std::lock_guard<jw::QuickMutex> g(_mutex);
                (void)g;

                _sessions.erase(s);
            }
        }

    private:
        std::unordered_set<std::shared_ptr<Session> > _sessions;
        jw::QuickMutex _mutex;
    };

    typedef jw::BasicServer<ServerProxy, 128> Server;
}

#include <iostream>

int main() {
    jw::IOService<gs::Server> s(8899);
    (void)s;
    getchar();
    return 0;

    auto te = jw::TimerEngine::getInstance();
    te->registerTimer(1, std::chrono::milliseconds(1000), jw::TimerEngine::REPEAT_FOREVER, [&te](int64_t dt) {
        LOG_INFO("timer dt = %I64d", dt);
        te->unregisterTimer(1);
        te->registerTimer(2, std::chrono::milliseconds(500), 20, [](int64_t dt) { LOG_INFO("dt = %I64d", dt); });
    });
    getchar();
    jw::TimerEngine::destroyInstance();

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
