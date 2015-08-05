#ifndef _PACKET_SPLITTER_H_
#define _PACKET_SPLITTER_H_

#include "DebugConfig.h"
#include "../json-test/cppJSON.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>

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
            unsigned recvLen = (unsigned char)_cacheBuf[3];
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

            _cacheBuf.erase(_cacheBuf.begin(), _cacheBuf.begin() + 4);
            buf = std::move(_cacheBuf);  // 移动到buf里面
            if (bufSize - 4 > recvLen) {
                // 将多余的保存在cache里面，并从buf中移走多余的
                std::copy(buf.begin() + 4 + recvLen, buf.end(), std::back_inserter(_cacheBuf));
                buf.erase(buf.begin() + 4 + recvLen, buf.end());
                LOG_DEBUG("to long for packet body, remain : %lu", (unsigned long)bufSize - 4 - recvLen);
            }
        }

        std::vector<char> decodeRecvPacket(const char *data, size_t length) {
            std::vector<char> buf;
            decodeRecvPacket(buf, data, length);
            return std::move(buf);
        }

        static void encodeSendPacket(std::vector<char> &buf, const std::string &str) {
            buf.clear();
            size_t sendLen = str.length();  // 包体长度
            buf.reserve(sendLen + 4);
            buf.push_back((sendLen >>  0) & 0xFF);
            buf.push_back((sendLen >>  8) & 0xFF);
            buf.push_back((sendLen >> 16) & 0xFF);
            buf.push_back((sendLen >> 24) & 0xFF);
            std::copy(str.begin(), str.end(), std::back_inserter(buf));
        }

        static std::vector<char> encodeSendPacket(const std::string &str) {
            std::vector<char> buf;
            encodeSendPacket(buf, str);
            return buf;
        }
    };

    struct JsonPacketSplitter : PacketSplitter {
        void decodeRecvPacket(jw::cppJSON &json, unsigned &cmd, unsigned &tag, const char *data, size_t length) {
            std::vector<char> buf;
            PacketSplitter::decodeRecvPacket(buf, data, length);

            size_t size = buf.size();
            if (size >= 8) {
                cmd = (unsigned char)buf[3];
                cmd <<= 8;
                cmd |= (unsigned char)buf[2];
                cmd <<= 8;
                cmd |= (unsigned char)buf[1];
                cmd <<= 8;
                cmd |= (unsigned char)buf[0];

                tag = (unsigned char)buf[7];
                tag <<= 8;
                tag |= (unsigned char)buf[6];
                tag <<= 8;
                tag |= (unsigned char)buf[5];
                tag <<= 8;
                tag |= (unsigned char)buf[4];
            }

            if (size <= 8) {
                cmd = 0;
                tag = (unsigned)-1;
                json.clear();
            }
            else {
                LOG_DEBUG("cmd = %u tag = %u | %.*s", cmd, tag, (int)size - 8, &buf[8]);
                json.Parse(&buf[8]);
            }
        }

        static std::vector<char> encodeSendPacket(unsigned cmd, unsigned tag, const jw::cppJSON &json) {
            std::vector<char> buf(12);
            json.PrintTo(buf, false);

            size_t length = buf.size() - 4;  // 包体长度
            buf[0] = ((length >>  0) & 0xFF);
            buf[1] = ((length >>  8) & 0xFF);
            buf[2] = ((length >> 16) & 0xFF);
            buf[3] = ((length >> 24) & 0xFF);

            buf[4] = ((cmd >>  0) & 0xFF);
            buf[5] = ((cmd >>  8) & 0xFF);
            buf[6] = ((cmd >> 16) & 0xFF);
            buf[7] = ((cmd >> 24) & 0xFF);

            buf[ 8] = ((tag >>  0) & 0xFF);
            buf[ 9] = ((tag >>  8) & 0xFF);
            buf[10] = ((tag >> 16) & 0xFF);
            buf[11] = ((tag >> 24) & 0xFF);

            LOG_DEBUG("%.*s", (int)length - 8, !json.empty() ? &buf[12] : "");
            return buf;
        }

        static void modifyTag(std::vector<char> &buf, unsigned tag) {
            if (buf.size() < 12) {
                throw std::out_of_range("buf size should be greater than 12");
            }
            buf[ 8] = ((tag >>  0) & 0xFF);
            buf[ 9] = ((tag >>  8) & 0xFF);
            buf[10] = ((tag >> 16) & 0xFF);
            buf[11] = ((tag >> 24) & 0xFF);
        }
    };
}

#endif
