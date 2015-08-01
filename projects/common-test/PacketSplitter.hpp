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
        // 解出的包从buf中传回
        void decodeRecvPacket(std::vector<char> &buf, const char *data, size_t length) {
            std::copy(data, data + length, std::back_inserter(_cacheBuf));  // 将data复制到cache尾部
            size_t bufSize = _cacheBuf.size();
            if (bufSize < 4) {  // 包头
                LOG_DEBUG("not enough for packet header");
                buf.clear();
                return;
            }

            // 计算包体长度
            size_t recvLen = (unsigned char)_cacheBuf[3];
            recvLen <<= 8;
            recvLen |= (unsigned char)_cacheBuf[2];
            recvLen <<= 8;
            recvLen |= (unsigned char)_cacheBuf[1];
            recvLen <<= 8;
            recvLen |= (unsigned char)_cacheBuf[0];
            if (bufSize - 4 < recvLen) {
                LOG_DEBUG("not enough for packet body, expect : %lu", (unsigned long)recvLen);
                buf.clear();
                return;
            }

            buf = std::move(_cacheBuf);  // 移动到buf里面
            if (bufSize - 4 > recvLen) {
                // 将多余的保存在cache里面，并从buf中移走多余的
                std::copy(buf.begin() + 4 + recvLen, buf.end(), std::back_inserter(_cacheBuf));
                buf.erase(buf.begin() + 4 + recvLen, buf.end());
                LOG_DEBUG("to long for packet body, remain : %lu", (unsigned long)bufSize - 4 - recvLen);
            }
            LOG_DEBUG("%.*s", (int)buf.size() - 4, &buf[4]);
        }

        std::vector<char> decodeRecvPacket(const char *data, size_t length) {
            std::vector<char> buf;
            decodeRecvPacket(buf, data, length);
            return std::move(buf);
        }

        void encodeSendPacket(std::vector<char> &buf, const std::string &str) {
            buf.clear();
            size_t sendLen = str.length();  // 包体长度
            buf.reserve(sendLen + 4);
            buf.push_back((sendLen >> 0) & 0xFF);
            buf.push_back((sendLen >> 8) & 0xFF);
            buf.push_back((sendLen >> 16) & 0xFF);
            buf.push_back((sendLen >> 24) & 0xFF);
            std::copy(str.begin(), str.end(), std::back_inserter(buf));
        }

        std::vector<char> encodeSendPacket(const std::string &str) {
            std::vector<char> buf;
            encodeSendPacket(buf, str);
            return buf;
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

        jw::cppJSON decodeRecvPacket(const char *data, size_t length) {
            jw::cppJSON ret;
            decodeRecvPacket(ret, data, length);
            return ret;
        }

        void encodeSendPacket(std::vector<char> &buf, const jw::cppJSON &json) {
            std::string str = json.PrintUnformatted();
            LOG_DEBUG("%s", str.c_str());
            PacketSplitter::encodeSendPacket(buf, str);
        }

        std::vector<char> encodeSendPacket(const jw::cppJSON &json) {
            std::vector<char> buf;
            encodeSendPacket(buf, json);
            return buf;
        }
    };
}

#endif
