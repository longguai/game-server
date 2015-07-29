#ifndef _BASIC_ROOM_HPP_
#define _BASIC_ROOM_HPP_

#include "BasicTable.hpp"
#include "QuickMutex.h"

namespace gs {
    template <class _GameUser, class _GameLogic, size_t _TableCount>
    class BasicRoom {
        std::vector<_GameUser> _userList;
        BasicTable<_GameUser, _GameLogic> _desk[_TableCount];
        jw::QuickMutex _mutex;

    public:
        bool joinIn(int64_t id) {
            try {
                _GameUser user;
                user.name = std::to_string(id);
                user.desk = -1;
                user.seat = -1;
                user.status = UserStatus::Free;

                user.winCount = 0;
                user.tieCount = 0;
                user.loseCount = 0;
                user.scores = 0;

                std::string msg = user.encodeJoinIn();
                const void *data = msg.c_str();
                size_t size = msg.length();

                std::lock_guard<jw::QuickMutex> g(_mutex);
                (void)g;

                _userList.push_back(user);
                std::for_each(_userList.begin(), _userList.end() - 1, std::bind(&_GameUser::deliver, std::placeholders::_1, data, size));
                return true;
            }
            catch (std::exception &e) {
                LOG_ERROR("%s", e.what());
                return false;
            }
        }

        bool leave(_GameUser *user) {
            std::vector<_GameUser>::iterator it = std::find_if(_userList.begin(), _userList.end(), [user](_GameUser &each) { return &each == user; });
            if (it == _userList.end()) {
                return false;
            }
            try {
                std::string msg = user->encodeLeave();
                const void *data = msg.c_str();
                size_t size = msg.length();

                std::lock_guard<jw::QuickMutex> g(_mutex);
                (void)g;

                _userList.erase(it);
                std::for_each(_userList.begin(), _userList.end(), std::bind(&_GameUser::deliver, std::placeholders::_1, data, size));
                return true;
            }
            catch (std::exception &e) {
                LOG_ERROR("%s", e.what());
                return false;
            }
        }

        bool sitDown(_GameUser *user) {
            return true;
        }

        bool standUp(_GameUser *user) {
            return true;
        }

        bool handsUp(_GameUser *user) {
            return true;
        }
    };

}

#endif
