#ifndef _BASIC_ROOM_HPP_
#define _BASIC_ROOM_HPP_

#include "BasicTable.hpp"
#include "QuickMutex.h"
#include "PacketSplitter.hpp"
#include <unordered_set>

namespace gs {
    struct ConnectedUser : jw::JsonPacketSplitter {
        int64_t id;
        std::string name;
    };

    template <class _GameTable, size_t _TableCount>
    class BasicRoom {
    public:
        typedef typename _GameTable::UserType UserType;

    private:
        std::unordered_set<std::shared_ptr<UserType> > _userSet;
        _GameTable _table[_TableCount];
        jw::QuickMutex _mutex;

    public:
        void deliver(const std::shared_ptr<UserType> &user, jw::cppJSON &json) { }
    };
}

#endif
