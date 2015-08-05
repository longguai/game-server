#ifndef _GAME_TABLE_H_
#define _GAME_TABLE_H_

#include "../common-test/BasicSession.hpp"
#include "../common-test/BasicTable.hpp"
#include "../common-test/ConnectedUser.hpp"
#include "U5TKLogic.h"

enum class UserStatus {
    Free = 0,
    HandsUp = 1,
    Playing = 2
};

struct RoomUserBase : gs::ConnectedUser {
    int table = -1;
    int seat = -1;
    UserStatus status = UserStatus::Free;
    uint32_t winCount = 0;
    uint32_t tieCount = 0;
    uint32_t loseCount = 0;
    int32_t scores = 0;
};

typedef jw::BasicSession<RoomUserBase, 1024U> GameUser;

class GameTable : public gs::BasicTable<GameUser, U5TKLogic> {
public:
    bool handsUp(unsigned seat);
    void deliver(unsigned seat, unsigned cmd, unsigned tag, const jw::cppJSON &json);
    void forcedStandUp(unsigned seat);

private:
    void _sendGameState();
    void _handleLogicResult(unsigned seat, unsigned cmd, unsigned tag, U5TKLogic::ErrorType errorType);
};

#endif
