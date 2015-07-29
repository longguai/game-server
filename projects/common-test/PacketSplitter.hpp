#ifndef _PACKET_SPLITTER_H_
#define _PACKET_SPLITTER_H_

#include "DebugConfig.h"
#include "../json-test/cppJSON.hpp"
#include <vector>
#include <algorithm>

namespace jw {
    class PacketSplitter {
    private:
        std::vector<char> _cacheBuf;

    public:
        void decodeRecvPacket(std::vector<char> &buf, const char *data, size_t length) {
            std::copy(data, data + length, std::back_inserter(_cacheBuf));
            size_t bufSize = _cacheBuf.size();
            if (bufSize < 4) {
                LOG_DEBUG("包头不够");
                buf.clear();
                return;
            }

            size_t recvLen = (unsigned char)_cacheBuf[3];
            recvLen <<= 8;
            recvLen |= (unsigned char)_cacheBuf[2];
            recvLen <<= 8;
            recvLen |= (unsigned char)_cacheBuf[1];
            recvLen <<= 8;
            recvLen |= (unsigned char)_cacheBuf[0];
            if (bufSize - 4 < recvLen) {
                LOG_DEBUG("包体不够，目标%lu", (unsigned long)recvLen);
                buf.clear();
                return;
            }

            buf = std::move(_cacheBuf);
            if (bufSize - 4 > recvLen) {
                std::copy(buf.begin() + 4 + recvLen, buf.end(), std::back_inserter(_cacheBuf));
                buf.erase(buf.begin() + 4 + recvLen, buf.end());
                LOG_DEBUG("包体太长，余下%lu", (unsigned long)bufSize - 4 - recvLen);
            }
            LOG_DEBUG("%.*s", (int)buf.size() - 4, &buf[4]);
        }


        void encodeSendPacket(std::vector<char> &buf, const std::string &str) {
            buf.clear();
            size_t sendLen = str.length();
            buf.reserve(sendLen + 4);
            buf.push_back((sendLen >> 0) & 0xFF);
            buf.push_back((sendLen >> 8) & 0xFF);
            buf.push_back((sendLen >> 16) & 0xFF);
            buf.push_back((sendLen >> 24) & 0xFF);
            std::copy(str.begin(), str.end(), std::back_inserter(buf));
        }
    };

    struct JsonPacketSplitter : PacketSplitter {
        void decodeRecvPacket(jw::cppJSON &json, const char *data, size_t length) {
            std::vector<char> buf;
            PacketSplitter::decodeRecvPacket(buf, data, length);
            if (buf.empty()) {
                json.clear();
            }
            else {
                json.Parse(&buf[4]);
            }
        }

        void encodeSendPacket(std::vector<char> &buf, const jw::cppJSON &json) {
            std::string str = json.PrintUnformatted();
            LOG_DEBUG("%s", str.c_str());
            return PacketSplitter::encodeSendPacket(buf, str);
        }
    };
}

#endif
