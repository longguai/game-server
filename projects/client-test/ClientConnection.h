#ifndef _CLIENT_CONNECTION_H_
#define _CLIENT_CONNECTION_H_

#if (defined _WIN32) || (defined WIN32)
#   define PLATFORM_IS_WINDOWS 1
#   include <winsock2.h>
#   include <mswsock.h>
#   include <windows.h>
#else
#   define PLATFORM_IS_WINDOWS 0
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   define INVALID_SOCKET (-1)
#   define SOCKET_ERROR (-1)
typedef int SOCKET;
#endif

#include <thread>
#include <vector>
#include "SocketRecvBuffer.hpp"
#include "SocketSendBuffer.hpp"

namespace jw {

    class ClientConnection final {

    public:
        ClientConnection();
        ~ClientConnection();

        void connentToServer(const char *ip, unsigned short port);
        void quit();

        bool isWaiting() const { return _isWaiting; }
        bool isConnectSuccess() const { return _isConnectSuccess; }
        int writeBuf(const char *inBuf, int len);
        int readBuf(char *outBuf, int maxLen);

    private:
        ClientConnection(const ClientConnection &);
        ClientConnection(ClientConnection &&);
        ClientConnection &operator=(const ClientConnection &);
        ClientConnection &operator=(ClientConnection &&);

        SOCKET                          _socket = INVALID_SOCKET;
        std::thread                     *_connectThread = nullptr;
        volatile bool                   _isWaiting = true;
        volatile bool                   _isConnectSuccess = false;

        std::thread                     *_sendThread = nullptr;
        volatile bool                   _sendNeedQuit = false;
        SocketSendBuffer<1024>          _sendBuf;

        std::thread                     *_recvThread = nullptr;
        volatile bool                   _recvNeedQuit = false;
        SocketRecvBuffer<1024>          _recvBuf;

        void _connectToServer(const char *ip, unsigned short port);
    };
}

#endif
