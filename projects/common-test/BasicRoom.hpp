#ifndef _BASIC_ROOM_HPP_
#define _BASIC_ROOM_HPP_

#include "BasicTable.hpp"
#include "QuickMutex.h"
#include <unordered_set>

namespace gs {
    template <class _GameTable, size_t _TableCount>
    class BasicRoom {
    public:
        typedef _GameTable TableType;
        typedef typename _GameTable::UserType UserType;
        static const size_t TableCount = _TableCount;

        //void deliver(const std::shared_ptr<UserType> &user, const jw::cppJSON &json) { }

        //void addUser(const std::shared_ptr<UserType> &user) {
        //    std::lock_guard<jw::QuickMutex> g(_mutex);
        //    (void)g;
        //    _userSet.insert(user);
        //}

        //void removeUser(const std::shared_ptr<UserType> &user) {
        //    std::lock_guard<jw::QuickMutex> g(_mutex);
        //    (void)g;
        //    _userSet.erase(user);
        //}

    protected:
        std::unordered_set<std::shared_ptr<UserType> > _userSet;
        _GameTable _table[_TableCount];
        jw::QuickMutex _mutex;
    };
}

#endif
