#include "GameRoom.h"

#define CMD_ENTER 3000
#define CMD_SIT_DOWN 3001
#define CMD_STAND_UP 3002
#define CMD_HANDS_UP 3003
#define CMD_CHAT_IN_ROOM 3004
#define CMD_FORCED_STAND_UP 3005

void GameRoom::deliver(const std::shared_ptr<UserType> &user, unsigned cmd, unsigned tag, const jw::cppJSON &jsonRecv) {
    try {
        switch (cmd) {
        case CMD_ENTER: handleEnter(cmd, tag, user, jsonRecv); return;
        case CMD_SIT_DOWN: handleSitDown(cmd, tag, user, jsonRecv); return;
        case CMD_STAND_UP: handleStandUp(cmd, tag, user, jsonRecv); return;
        case CMD_HANDS_UP: handleHandsUp(cmd, tag, user, jsonRecv); return;
        case CMD_CHAT_IN_ROOM: handleChatInRoom(cmd, tag, user, jsonRecv); return;
        default: handleTableAction(cmd, tag, user, jsonRecv); return;
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}

void GameRoom::addUser(const std::shared_ptr<UserType> &user) {
    std::lock_guard<jw::QuickMutex> g(_mutex);
    (void)g;
    user->name = user->getRemoteIP() + ":" + std::to_string(user->getRemotePort());
    user->id = std::chrono::system_clock::now().time_since_epoch().count();
    _userSet.insert(user);
    LOG_INFO("%I64d", user->id);
}

void GameRoom::removeUser(const std::shared_ptr<UserType> &user) {
    std::lock_guard<jw::QuickMutex> g(_mutex);
    (void)g;
    int table = user->table;
    int seat = user->seat;
    int64_t id = user->id;
    if (isValidTable(table, seat)) {
        _table[table].forcedStandUp(seat);
    }
    _userSet.erase(user);

    jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
    jsonSend.insert(std::make_pair("id", user->id));
    jsonSend.insert(std::make_pair("table", table));
    jsonSend.insert(std::make_pair("seat", seat));
    std::vector<char> buf = user->encodeSendPacket(CMD_FORCED_STAND_UP, PUSH_SERVICE_TAG, jsonSend);
    std::for_each(_userSet.begin(), _userSet.end(), [&buf](const std::shared_ptr<UserType> &s) {
        s->deliver(buf);
    });
}

void GameRoom::handleEnter(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv) {
    try {
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;

        jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
        std::pair<jw::cppJSON::iterator, bool> ret = jsonSend.insert(std::make_pair("users", jw::cppJSON(jw::cppJSON::ValueType::Array)));
        if (ret.second) {
            std::for_each(_userSet.begin(), _userSet.end(), [&ret](const std::shared_ptr<UserType> &user) {
                jw::cppJSON json(jw::cppJSON::ValueType::Object);
                json.insert(std::make_pair("id", user->id));
                json.insert(std::make_pair("name", user->name));
                json.insert(std::make_pair("table", user->table));
                json.insert(std::make_pair("seat", user->seat));
                json.insert(std::make_pair("status", user->status));
                json.insert(std::make_pair("winCount", user->winCount));
                json.insert(std::make_pair("tieCount", user->tieCount));
                json.insert(std::make_pair("loseCount", user->loseCount));
                json.insert(std::make_pair("scores", user->scores));
                ret.first->push_back(std::move(json));
            });
        }
        std::vector<char> buf = user->encodeSendPacket(cmd, PUSH_SERVICE_TAG, jsonSend);
        std::for_each(_userSet.begin(), _userSet.end(), [&buf, &user](const std::shared_ptr<UserType> &s) {
            if (s != user) {
                s->deliver(buf);
            }
        });
        jsonSend.insert(std::make_pair("yourId", user->id));
        buf = user->encodeSendPacket(cmd, tag, jsonSend);
        user->deliver(buf);
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}

void GameRoom::handleSitDown(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv) {
    try {
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;

        unsigned table = jsonRecv.getValueByKey<unsigned>("table");
        unsigned seat = jsonRecv.getValueByKey<unsigned>("seat");

        jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
        if (!isValidTable(table, seat)) {
            jsonSend.insert(std::make_pair("result", false));
            jsonSend.insert(std::make_pair("reason", u8"非法的位置"));
            user->deliver(user->encodeSendPacket(cmd, tag, jsonSend));
        }
        else if (!_table[table].sitDown(user, seat)) {
            jsonSend.insert(std::make_pair("result", false));
            jsonSend.insert(std::make_pair("reason", u8"这个位置已经有人了"));
            user->deliver(user->encodeSendPacket(cmd, tag, jsonSend));
        }
        else {
            if (user->table != -1 && user->seat != -1) {
                _table[table].standUp(user, seat);
            }
            user->table = table;
            user->seat = seat;
            jsonSend.insert(std::make_pair("result", true));
            jsonSend.insert(std::make_pair("id", user->id));
            jsonSend.insert(std::make_pair("table", table));
            jsonSend.insert(std::make_pair("seat", seat));
            std::vector<char> buf = user->encodeSendPacket(cmd, PUSH_SERVICE_TAG, jsonSend);
            std::for_each(_userSet.begin(), _userSet.end(), [&buf, &user](const std::shared_ptr<UserType> &s) {
                if (user != s) {
                    s->deliver(buf);
                }
            });

            std::pair<jw::cppJSON::iterator, bool> ret = jsonSend.insert(std::make_pair("participants", jw::cppJSON(jw::cppJSON::ValueType::Array)));
            if (ret.second) {
                const std::shared_ptr<UserType> (&participants)[4] = _table[table].getParticipants();
                std::for_each(std::begin(participants), std::end(participants), [&ret](const std::shared_ptr<UserType> &user) {
                    ret.first->push_back((user != nullptr) ? user->id : 0);
                });
            }
            buf = user->encodeSendPacket(cmd, tag, jsonSend);
            user->deliver(buf);
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}

void GameRoom::handleStandUp(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv) {
    try {
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;

        jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
        if (!isValidTable(user->table, user->seat) || !_table[user->table].standUp(user, user->seat)) {
            jsonSend.insert(std::make_pair("result", false));
            jsonSend.insert(std::make_pair("reason", u8"你不在桌子上"));
            user->deliver(user->encodeSendPacket(cmd, tag, jsonSend));
        }
        else {
            user->table = -1;
            user->seat = -1;
            jsonSend.insert(std::make_pair("result", true));
            jsonSend.insert(std::make_pair("id", user->id));
            std::vector<char> buf = user->encodeSendPacket(cmd, PUSH_SERVICE_TAG, jsonSend);
            std::for_each(_userSet.begin(), _userSet.end(), [&buf, &user](const std::shared_ptr<UserType> &s) {
                if (s != user) {
                    s->deliver(buf);
                }
            });
            user->modifyTag(buf, tag);
            user->deliver(buf);
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}

void GameRoom::handleHandsUp(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv) {
    try {
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;

        jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
        jsonSend.insert(std::make_pair("cmd", cmd));
        if (!isValidTable(user->table, user->seat)) {
            jsonSend.insert(std::make_pair("result", false));
            jsonSend.insert(std::make_pair("reason", u8"你不在桌子上"));
            user->deliver(user->encodeSendPacket(cmd, tag, jsonSend));
        }
        else if (!_table[user->table].handsUp(user->seat)) {
            jsonSend.insert(std::make_pair("result", false));
            jsonSend.insert(std::make_pair("reason", u8"当前状态不允许准备"));
            user->deliver(user->encodeSendPacket(cmd, tag, jsonSend));
        }
        else {
            jsonSend.insert(std::make_pair("result", true));
            jsonSend.insert(std::make_pair("id", user->id));
            std::vector<char> buf = user->encodeSendPacket(cmd, PUSH_SERVICE_TAG, jsonSend);
            std::for_each(_userSet.begin(), _userSet.end(), [&buf, &user](const std::shared_ptr<UserType> &s) {
                if (s != user) {
                    s->deliver(buf);
                }
            });
            user->modifyTag(buf, tag);
            user->deliver(buf);
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}

void GameRoom::handleChatInRoom(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv) {
    try {
        std::string content = jsonRecv.getValueByKey<std::string>("content");

        jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
        jsonSend.insert(std::make_pair("ip", user->getRemoteIP()));
        jsonSend.insert(std::make_pair("port", user->getRemotePort()));
        jsonSend.insert(std::make_pair("content", content));

        std::vector<char> buf = user->encodeSendPacket(cmd, PUSH_SERVICE_TAG, jsonSend);
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;
        std::for_each(_userSet.begin(), _userSet.end(), [&buf](const std::shared_ptr<UserType> &s) {
            s->deliver(buf);
        });
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}

void GameRoom::handleTableAction(unsigned cmd, unsigned tag, const std::shared_ptr<UserType> &user, const jw::cppJSON &jsonRecv) {
    try {
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;

        if (!isValidTable(user->table, user->seat)) {
            jw::cppJSON jsonSend(jw::cppJSON::ValueType::Object);
            jsonSend.insert(std::make_pair("result", false));
            jsonSend.insert(std::make_pair("reason", u8"你不在桌子上"));
            std::vector<char> buf = user->encodeSendPacket(cmd, tag, jsonSend);
            user->deliver(buf);
        }
        else {
            _table[user->table].deliver(user->seat, cmd, tag, jsonRecv);
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }
}
