#ifndef _GAME_SERVER_H_
#define _GAME_SERVER_H_

#include "../common-test/BasicServer.hpp"
#include "GameRoom.h"

class ServerProxy {
public:
    typedef GameRoom::UserType Session;

    void acceptCallback(asio::ip::tcp::socket &&socket) {
        std::shared_ptr<Session> s = std::make_shared<Session>(std::move(socket),
            std::bind(&ServerProxy::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _room.addUser(s);
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
                _room.deliver(s, cmd, tag, jsonRecv);
            }
            catch (std::exception &e) {
                LOG_ERROR("%s", e.what());
                _room.removeUser(s);
            }
        }
        else {
            _room.removeUser(s);
        }
    }

private:
    GameRoom _room;
};

typedef jw::BasicServer<ServerProxy, 128> Server;

#endif
