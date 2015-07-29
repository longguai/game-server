#include "ClientConnection.h"

#include <iostream>

#undef min
#undef max

#include "../json-test/cppJSON.hpp"

int main() {

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
                //printf("%.*s\n", ret, buf);

                try {
                    jw::cppJSON jsonRecv;
                    jsonRecv.Parse(buf);
                    std::string ip = jsonRecv.findAs<std::string>("ip");
                    unsigned short port = jsonRecv.findAs<unsigned short>("port");
                    std::string content = jsonRecv.findAs<std::string>("content");
                    printf("%s:%hu 说：%s\n", ip.c_str(), port, content.c_str());
                }
                catch (std::exception &e) {
                    printf("%s\n", e.what());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    {
        char str[1024] = "Hello World!";
        size_t len = strlen(str);
        while (scanf("%[^\n]%*c", str) != EOF) {
            try {
                jw::cppJSON json(jw::cppJSON::ValueType::Object);
                json.insert(std::make_pair("content", str));

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
