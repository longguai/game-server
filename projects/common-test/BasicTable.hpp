#ifndef _GAME_TABLE_HPP_
#define _GAME_TABLE_HPP_

#include <stddef.h>
#include <string.h>
#include <vector>

namespace gs {
    template <size_t _ParticipantCount> struct BasicLogic {
        static const size_t ParticipantCount = _ParticipantCount;
    };

    template <class _GameUser, class _GameLogic>
    struct BasicTable : public _GameLogic {
        BasicTable<_GameUser, _GameLogic>() {
            memset(participants, 0, sizeof(participants));
        }

        _GameUser *participants[_GameLogic::ParticipantCount];
        std::vector<_GameUser *> watchers;
    };
}

#endif
