#ifndef _CONNECTED_USER_HPP_
#define _CONNECTED_USER_HPP_

#include "PacketSplitter.hpp"

namespace gs {
    struct ConnectedUser : jw::JsonPacketSplitter {
        int64_t id = 0;
        std::string name;
    };
}

#endif
