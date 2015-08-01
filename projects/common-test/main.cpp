#include "BasicServer.hpp"
#include "BasicSession.hpp"

//namespace echo {
//    using jw::Session;
//    class ServerProxy {
//    public:
//        void acceptCallback(asio::ip::tcp::socket &&socket) {
//            std::shared_ptr<Session> s = std::make_shared<Session>(std::move(socket),
//                std::bind(&ServerProxy::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
//            s->start();
//        }
//
//    private:
//        void _sessionCallback(const std::shared_ptr<Session> &s, jw::SessionEvent event, const char *data, size_t length) {
//            if (data != nullptr) {
//                s->deliver(data, length);
//            }
//        }
//    };
//
//    typedef jw::BasicServer<ServerProxy, 128> Server;
//}

#include <unordered_set>

//namespace chat {
//    using jw::Session;
//    class ServerProxy {
//    public:
//        void acceptCallback(asio::ip::tcp::socket &&socket) {
//            std::shared_ptr<Session> s = std::make_shared<Session>(std::move(socket),
//                std::bind(&ServerProxy::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
//            _mutex.lock();
//            _sessions.insert(s);
//            _mutex.unlock();
//            s->start();
//        }
//
//    private:
//        void _sessionCallback(const std::shared_ptr<Session> &s, jw::SessionEvent event, const char *data, size_t length) {
//            if (data != nullptr) {
//                _mutex.lock();
//                std::for_each(_sessions.begin(), _sessions.end(), [data, length](const std::shared_ptr<Session> &s) {
//                    s->deliver(data, length);
//                });
//                _mutex.unlock();
//            }
//            else {
//                _mutex.lock();
//                _sessions.erase(s);
//                _mutex.unlock();
//            }
//        }
//
//    private:
//        std::unordered_set<std::shared_ptr<Session> > _sessions;
//        std::mutex _mutex;
//    };
//
//    typedef jw::BasicServer<ServerProxy, 128> Server;
//}

#include "DebugConfig.h"
#include <thread>
#include <chrono>
#include <stdio.h>

#include "TimerEngine.h"

#include "IOService.hpp"

#include "BasicRoom.hpp"
#include "PacketSplitter.hpp"

namespace gs {
    enum class UserStatus {
        Free = 0,
        HandsUp = 1,
        Playing = 2
    };

    struct RoomUserBase : ConnectedUser {
        int desk = -1;
        int seat = -1;
        UserStatus status = UserStatus::Free;
        uint32_t winCount = 0;
        uint32_t tieCount = 0;
        uint32_t loseCount = 0;
        int32_t scores = 0;

        std::string encodeJoinIn() {
            return "";
        }
        std::string encodeLeave() {
            return "";
        }
    };

    typedef jw::BasicSession<RoomUserBase, 1024U> Session;

    typedef jw::BasicSession<RoomUserBase, 1024U> GameUser;
    typedef gs::BasicLogic<4> GameLogic;
    typedef gs::BasicTable<GameUser, GameLogic> GameTable;
    typedef gs::BasicRoom<GameTable, 100> GameRoom;

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
                    jw::cppJSON jsonRecv = s->decodeRecvPacket(data, length);
                    if (jsonRecv == nullptr) {
                        return;
                    }

                    std::string content = jsonRecv.findAs<std::string>("content");

                    jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
                    jsonSend.insert(std::make_pair("ip", s->getRemoteIP()));
                    jsonSend.insert(std::make_pair("port", s->getRemotePort()));
                    jsonSend.insert(std::make_pair("content", content));

                    std::vector<char> buf = s->encodeSendPacket(jsonSend);

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

        GameRoom _room;
    };

    typedef jw::BasicServer<ServerProxy, 128> Server;
}

#include <iostream>

int main() {
    jw::IOService<gs::Server> s(8899);
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
