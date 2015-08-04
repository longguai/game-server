#include "ClientConnection.h"

#include <iostream>

#undef min
#undef max

#include "../json-test/cppJSON.hpp"

static const char *debugString(unsigned ca) {
    const char *table[5][11] = {
        { "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK", "DA", "D2" },
        { "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK", "CA", "C2" },
        { "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK", "HA", "H2" },
        { "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ", "SK", "SA", "S2" },
        { "JB", "JR" }
    };
#define GET_RANK(ca)    (((ca) >>  0U) & 0x0000000FU)
#define GET_SUIT(ca)    (((ca) >>  8U) & 0x0000000FU)
    unsigned suit = GET_SUIT(ca), rank = GET_RANK(ca);
    return suit < 5 ? table[suit - 1][rank - 5] : table[4][rank - 14];
}

int main() {
    system("chcp 65001");
    jw::ClientConnection cc;
    //cc.connentToServer("192.168.0.104", 8899);
    cc.connentToServer("127.0.0.1", 8899);

    std::thread t([&cc]() {
        char head[4];
        char buf[1024];
        while (1) {
            int ret = cc.readBuf(head, 4);
            if (ret == 4) {
                unsigned length = (unsigned char)head[3];
                length <<= 8;
                length |= (unsigned char)head[2];
                length <<= 8;
                length |= (unsigned char)head[1];
                length <<= 8;
                length |= (unsigned char)head[0];
                ret = cc.readBuf(buf, length);
                printf("%.*s\n", ret, buf);

                try {
                    jw::cppJSON jsonRecv;
                    jsonRecv.Parse(buf);

                    std::vector<std::vector<unsigned> > bringingCards = jsonRecv.getValueByKey<std::vector<std::vector<unsigned> > >("bringingCards");
                    puts("bringingCards:");
                    puts("---------------------------------------");
                    for (auto &bringingCard : bringingCards) {
                        for (unsigned ca : bringingCard) {
                            printf("%s(%4u) ", debugString(ca), ca);
                        }
                        puts("---------------------------------------");
                    }

                    std::vector<unsigned> cards = jsonRecv.getValueByKey<std::vector<unsigned> >("handCards");
                    puts("handCards");
                    puts("---------------------------------------");
                    std::for_each(cards.begin(), cards.end(), [](unsigned ca) {
                        printf("%s(%4u) ", debugString(ca), ca);
                    });
                    puts("\n---------------------------------------");
                    //std::string ip = jsonRecv.findAs<std::string>("ip");
                    //unsigned short port = jsonRecv.findAs<unsigned short>("port");
                    //std::string content = jsonRecv.findAs<std::string>("content");
                    //printf("%s:%hu 说：%s\n", ip.c_str(), port, content.c_str());
                }
                catch (std::exception &e) {
                    printf("%s\n", e.what());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    {
        char str[1024] = "";
        while (scanf("%[^\n]%*c", str) != EOF) {
            try {
                jw::cppJSON json(jw::cppJSON::ValueType::Object);
                json.Parse(str);
                //json.insert(std::make_pair("cmd", 3004));
                //json.insert(std::make_pair("content", str));

                std::string s = json.PrintUnformatted();
                size_t length = s.length();
                std::vector<char> buf;
                buf.push_back((length >> 0) & 0xFF);
                buf.push_back((length >> 8) & 0xFF);
                buf.push_back((length >> 16) & 0xFF);
                buf.push_back((length >> 24) & 0xFF);
                std::copy(s.begin(), s.end(), std::back_inserter(buf));
                cc.writeBuf(&buf.at(0), buf.size());
            }
            catch (std::exception &e) {
                printf("%s\n", e.what());
            }
        }
    }
    cc.quit();

    return 0;
}
