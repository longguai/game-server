#ifndef _GAME_TABLE_HPP_
#define _GAME_TABLE_HPP_

#include <stddef.h>
#include <vector>
#include <memory>

namespace gs {
    template <size_t _ParticipantCount> struct BasicLogic {
        static const size_t ParticipantCount = _ParticipantCount;
    };

    template <class _GameUser, class _GameLogic>
    struct BasicTable : public _GameLogic {
        typedef _GameUser UserType;
        BasicTable<_GameUser, _GameLogic>() {
        }

        //void deliver(const std::weak_ptr<_GameUser> &user, jw::cppJSON &json);

    private:
        std::weak_ptr<_GameUser> participants[_GameLogic::ParticipantCount];
        std::vector<std::weak_ptr<_GameUser> > watchers;
    };
}

#endif
