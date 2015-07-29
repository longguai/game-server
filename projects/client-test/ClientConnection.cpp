#include "ClientConnection.h"
#include <utility>
#include <assert.h>
#include <stdio.h>

#if PLATFORM_IS_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#endif

#define LOG_DEBUG printf

namespace jw {

#if PLATFORM_IS_WINDOWS
    struct WinSockLibLoader {
        WinSockLibLoader() {
            WSADATA data;
            WORD ver = MAKEWORD(2, 2);
            int ret = ::WSAStartup(ver, &data);
            if (ret != 0) {
                LOG_DEBUG("WSAStartup failed: last error %d", ::WSAGetLastError());
            }
        }
        ~WinSockLibLoader() {
            ::WSACleanup();
        }
    };
#endif

    ClientConnection::ClientConnection() {
#if PLATFORM_IS_WINDOWS
        static WinSockLibLoader loader;
#endif
    }

    ClientConnection::~ClientConnection() {
        quit();
    }

    void ClientConnection::connentToServer(const char *ip, unsigned short port) {
        if (_isConnectSuccess || _connectThread != nullptr) {
            return;
        }

        _socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_socket == INVALID_SOCKET) {
            return;
        }

        _isWaiting = true;
        _connectThread = new (std::nothrow) std::thread(std::bind(&ClientConnection::_connectToServer, this, ip, port));
    }

    void ClientConnection::quit() {
        if (_socket != INVALID_SOCKET) {
#if PLATFORM_IS_WINDOWS
            ::closesocket(_socket);
#else
            ::shutdown(_socket, SHUT_RDWR);
#endif
            _socket = INVALID_SOCKET;
        }

        if (_connectThread != nullptr) {
            _connectThread->join();
            delete _connectThread;
            _connectThread = nullptr;
        }

        if (_sendThread != nullptr) {
            _sendNeedQuit = true;
            _sendThread->join();
            delete _sendThread;
            _sendThread = nullptr;
        }

        if (_recvThread != nullptr) {
            _recvNeedQuit = true;
            _recvThread->join();
            delete _recvThread;
            _recvThread = nullptr;
        }
        _isConnectSuccess = false;
    }

    int ClientConnection::writeBuf(const char *inBuf, int len) {
        int freeSize = _sendBuf.freeSize();
        if (freeSize >= len) {
            return _sendBuf.write(inBuf, len);
        } else {
            return _sendBuf.write(inBuf, freeSize);
        }
    }

    int ClientConnection::readBuf(char *outBuf, int maxLen) {
        return _recvBuf.read(outBuf, maxLen);
    }

    void ClientConnection::_connectToServer(const char *ip, unsigned short port) {
        struct sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = ::inet_addr(ip);
        serverAddr.sin_port = htons(port);

        int ret = ::connect(_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr));
        if (ret == SOCKET_ERROR) {
            LOG_DEBUG("Cannot connect to %s:%hu\n", ip, port);
            _isWaiting = false;
            _isConnectSuccess = false;
            return;
        }
        LOG_DEBUG("connect to %s:%hu\n", ip, port);

        _isWaiting = false;
        _isConnectSuccess = true;

        _sendThread = new (std::nothrow) std::thread([this]() {
            while (!_sendNeedQuit) {
                int ret = _sendBuf.doSend(std::bind(&::send, _socket, std::placeholders::_1, std::placeholders::_2, 0));
                if (ret == SOCKET_ERROR) {
                    break;
                }
            }
        });

        _recvThread = new (std::nothrow) std::thread([this]() {
            while (!_recvNeedQuit) {
                int ret = _recvBuf.doRecv(std::bind(&::recv, _socket, std::placeholders::_1, std::placeholders::_2, 0));
                if (ret == SOCKET_ERROR) {
                    break;
                }
            }
        });
    }
}
