#ifndef _GAME_TABLE_HPP_
#define _GAME_TABLE_HPP_

#include "QuickMutex.h"
#include <stddef.h>
#include <vector>
#include <memory>

namespace gs {

    // 游戏逻辑
    // 不一定非要继承它，但是一定要有一个静态成员ParticipantCount表示参与玩家数量
    template <size_t _ParticipantCount> struct BasicLogic {
        static const size_t ParticipantCount = _ParticipantCount;
    };

    // 桌子
    template <class _GameUser, class _GameLogic>
    struct BasicTable : public _GameLogic {
        typedef _GameUser UserType;

        BasicTable<_GameUser, _GameLogic>() {
        }

        bool sitDown(const std::shared_ptr<_GameUser> &user, unsigned seat) {
            std::lock_guard<jw::QuickMutex> g(_mutex);
            (void)g;
            if (_participants[seat] != nullptr) {
                return false;
            }

            _participants[seat] = user;
            return true;
        }

        bool standUp(const std::shared_ptr<_GameUser> &user, unsigned seat) {
            std::lock_guard<jw::QuickMutex> g(_mutex);
            (void)g;
            if (_participants[seat] == nullptr) {
                return false;
            }
            _participants[seat].reset();
            return true;
        }

    protected:
        std::shared_ptr<_GameUser> _participants[_GameLogic::ParticipantCount];  // 参与玩家
        std::vector<std::shared_ptr<_GameUser> > _watchers;  // 旁观玩家

        jw::QuickMutex _mutex;
    };
}

#endif
