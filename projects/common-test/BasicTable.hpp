#ifndef _GAME_TABLE_HPP_
#define _GAME_TABLE_HPP_

#include "QuickMutex.h"
#include <stddef.h>
#include <vector>
#include <memory>

namespace gs {

    // 游戏逻辑
    // 不一定非要继承它，但是一定要有如下成员
    template <size_t _ParticipantCount> struct BasicLogic {
        static const size_t ParticipantCount = _ParticipantCount;
        void sitDown(unsigned seat) { }
        void standUp(unsigned seat) { }
    };

    // 桌子
    template <class _GameUser, class _GameLogic>
    struct BasicTable {
        typedef _GameUser UserType;
        typedef _GameLogic LogicType;
        static const size_t ParticipantCount = _GameLogic::ParticipantCount;

        BasicTable<_GameUser, _GameLogic>() {
        }

        bool sitDown(const std::shared_ptr<_GameUser> &user, unsigned seat) {
            std::lock_guard<jw::QuickMutex> g(_mutex);
            (void)g;
            if (_participants[seat] != nullptr) {
                return false;
            }

            _participants[seat] = user;
            _logic.sitDown(seat);
            return true;
        }

        bool standUp(const std::shared_ptr<_GameUser> &user, unsigned seat) {
            std::lock_guard<jw::QuickMutex> g(_mutex);
            (void)g;
            if (_participants[seat] == nullptr) {
                return false;
            }
            _participants[seat].reset();
            _logic.standUp(seat);
            return true;
        }

        // 语法：返回数组的引用
        std::shared_ptr<_GameUser> (&getParticipants())[_GameLogic::ParticipantCount] {
            return _participants;
        }

        const std::shared_ptr<_GameUser> (&getParticipants() const)[_GameLogic::ParticipantCount] {
            return _participants;
        }
    protected:
        std::shared_ptr<_GameUser> _participants[_GameLogic::ParticipantCount];  // 参与玩家
        std::vector<std::shared_ptr<_GameUser> > _watchers;  // 旁观玩家

        jw::QuickMutex _mutex;

        _GameLogic _logic;
    };
}

#endif
