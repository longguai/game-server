#ifndef _GAME_ROOM_H_
#define _GAME_ROOM_H_

#include "GameTable.h"
#include "../common-test/BasicRoom.hpp"

class GameRoom : public gs::BasicRoom<GameTable, 100> {
public:
    typedef gs::BasicRoom<GameTable, 100> BasicRoomType;
    typedef BasicRoomType::UserType UserType;

    void deliver(const std::shared_ptr<UserType> &user, unsigned cmd, unsigned tag, const jw::cppJSON &jsonRecv);
    void addUser(const std::shared_ptr<UserType> &user);
    void removeUser(const std::shared_ptr<UserType> &user);

private:
    static bool isValidTable(unsigned table, unsigned seat) {
        return table < BasicRoomType::TableCount && seat < BasicRoomType::TableType::ParticipantCount;
    }

    void handleEnter(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv);
    void handleSitDown(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv);
    void handleStandUp(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv);
    void handleHandsUp(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv);
    void handleChatInRoom(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv);
    void handleTableAction(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv);
};

#endif
