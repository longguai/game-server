#ifndef _IOCP_CORE_H_
#define _IOCP_CORE_H_

#if defined (_WINSOCKAPI_) && !defined (_WINSOCK2API_)
#   error winsock.h has already been included
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <stdint.h>
#include <vector>
#include <list>
#include <deque>
#include <utility>
#include <thread>
#include <mutex>
#include <algorithm>
#include "iocp-mutex.h"
#include "iocp-debug.h"
#include "iocp-exception.h"

#pragma comment(lib, "ws2_32.lib")
#pragma warning(push)
#pragma warning(disable: 4996)

#define MAX_POST_ACCEPT_COUNT 10
#define WORK_THREAD_RESERVE_SIZE 10
#define FREE_SOCKET_POOL_RESERVE_SIZE 128

#define CONTINUE_IF(_cond_) if (_cond_) continue
#define BREAK_IF(_cond_) if (_cond_) break

#define USE_CPP_EXCEPTION

#ifdef USE_CPP_EXCEPTION
#   define TRY_BLOCK_BEGIN try {
#   define CATCH_EXCEPTIONS } catch (std::exception &_e) { \
    LOG_ERROR("Caught exception `%s' at line %d in function `%s' of file `%s'", _e.what(), __LINE__, __FUNCTION__, __FILE__); if (1) {
#   define CATCH_BLOCK_END } }
#else
#   define TRY_BLOCK_BEGIN
#   define CATCH_EXCEPTIONS if (0) {
#   define CATCH_BLOCK_END }
#endif

namespace iocp {
    namespace _impl {
        template <template <class> class _ALLOC> class _ServerFramework;
        template <template <class> class _ALLOC> class _ClientContext;

        enum class _OPERATION_TYPE {
            NULL_POSTED = 0,
            ACCEPT_POSTED,
            RECV_POSTED,
            SEND_POSTED,
        };

        // 扩展OVERLAPPED结构体
        template <template <class> class _ALLOC>
        struct _BASE_OVERLAPPED : OVERLAPPED {
            _OPERATION_TYPE type;

            typedef void (*Operation)(_ServerFramework<_ALLOC> *server, _ClientContext<_ALLOC> *ctx, _BASE_OVERLAPPED<_ALLOC> *thiz, size_t bytesTransfered);
            Operation op;
        };

        template <template <class> class _ALLOC>
        struct _ACCEPT_OVERLAPPED : _BASE_OVERLAPPED<_ALLOC> {
            char buf[(sizeof(struct sockaddr_in) + 16) * 2];
            SOCKET clientSocket;
        };

        template <template <class> class _ALLOC>
        struct _PER_IO_OPERATION_DATA : _BASE_OVERLAPPED<_ALLOC> {
        };

        //
        // _ClientContext
        //
        template <template <class> class _ALLOC> class _ClientContext {  // 作为CompletionKey用
        private:
            SOCKET _socket = INVALID_SOCKET;

            _PER_IO_OPERATION_DATA<_ALLOC> _sendIOData;
            _PER_IO_OPERATION_DATA<_ALLOC> _recvIOData;
            //mutex _sendMutex;

            char _ip[16];
            uint16_t _port = 0;

            template <class _T> using list = std::list<_T, _ALLOC<_T> >;
            template <class _T> using vector = std::vector<_T, _ALLOC<_T> >;
            template <class _T> using deque = std::deque<_T, _ALLOC<_T> >;

            // 指向链表中自己的迭代器，这样做是基于std::list的迭代器不会因增删其他结点而失效
            typename list<_ClientContext *>::iterator _iterator;

            //vector<char> _sendCache;

            friend class _ServerFramework<_ALLOC>;

        private:
            _ClientContext(const _ClientContext &) = delete;
            _ClientContext(_ClientContext &&) = delete;
            _ClientContext &operator=(const _ClientContext &) = delete;
            _ClientContext &operator=(_ClientContext &&) = delete;

        protected:
            _ClientContext() {
            }

            ~_ClientContext() {
                if (_socket != INVALID_SOCKET) {
                    ::closesocket(_socket);
                    _socket = INVALID_SOCKET;
                }
            }

        public:
            inline const char *getIp() const { return _ip; }
            inline uint16_t getPort() const { return _port; }

            enum class POST_RESULT { SUCCESS, FAIL, CACHED };

            POST_RESULT asyncRecv(char *buf, size_t len, typename _BASE_OVERLAPPED<_ALLOC>::Operation op) {
                memset(&_recvIOData, 0, sizeof(OVERLAPPED));
                _recvIOData.type = _OPERATION_TYPE::RECV_POSTED;
                _recvIOData.op = op;
                DWORD bytesRecv = 0, flags = 0;

                WSABUF wsaBuf;
                wsaBuf.buf = buf;
                wsaBuf.len = len;
                int ret = ::WSARecv(_socket, &wsaBuf, 1, &bytesRecv, &flags, (LPOVERLAPPED)&_recvIOData, nullptr);
                if (ret == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING) {
                    return POST_RESULT::FAIL;
                }
                return POST_RESULT::SUCCESS;
            }

            POST_RESULT asyncSend(char *buf, size_t len, typename _BASE_OVERLAPPED<_ALLOC>::Operation op) {
                memset(&_sendIOData, 0, sizeof(OVERLAPPED));
                _sendIOData.type = _OPERATION_TYPE::SEND_POSTED;
                _sendIOData.op = op;

                DWORD bytesSent = 0;
                WSABUF wsaBuf;
                wsaBuf.buf = buf;
                wsaBuf.len = len;
                int ret = ::WSASend(_socket, &wsaBuf, 1, &bytesSent, 0, (LPOVERLAPPED)&_sendIOData, nullptr);
                if (ret == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING) {
                    return POST_RESULT::FAIL;
                }
                return POST_RESULT::SUCCESS;
            }

        };

        //
        // _ServerFramework
        //
        template <template <class> class _ALLOC> class _ServerFramework {

        private:
            template <class _T> using vector = _ClientContext<_ALLOC>::vector<_T>;
            template <class _T> using list = _ClientContext<_ALLOC>::list<_T>;
            template <class _T> using deque = _ClientContext<_ALLOC>::deque<_T>;

            char _ip[16];
            uint16_t _port = 0;

            HANDLE _ioCompletionPort = NULL;
            vector<std::thread *> _workerThreads;
            volatile bool _shouldQuit = false;

            SOCKET _listenSocket = INVALID_SOCKET;
            std::thread *_acceptThread = nullptr;
            vector<_ACCEPT_OVERLAPPED<_ALLOC> *> _allAcceptIOData;

            vector<SOCKET> _freeSocketPool;
            mutex _poolMutex;

            LPFN_ACCEPTEX _acceptEx = nullptr;
            LPFN_GETACCEPTEXSOCKADDRS _getAcceptExSockAddrs = nullptr;
            LPFN_DISCONNECTEX _disconnectEx = nullptr;

            mutex _clientMutex;
            list<_ClientContext<_ALLOC> *> _clientList;
            size_t _clientCount = 0;

        protected:
            std::function<_ClientContext<_ALLOC> *()> _allocateCtx;
            std::function<void (_ClientContext<_ALLOC> *ctx)> _deallocateCtx;

            std::function<void (_ClientContext<_ALLOC> *ctx)> _onAccept;
            std::function<void (_ClientContext<_ALLOC> *ctx)> _onDisconnect;

        private:
            _ServerFramework(const _ServerFramework &) = delete;
            _ServerFramework(_ServerFramework &&) = delete;
            _ServerFramework &operator=(const _ServerFramework &) = delete;
            _ServerFramework &operator=(_ServerFramework &&) = delete;

        public:
            _ServerFramework()
                : _workerThreads(WORK_THREAD_RESERVE_SIZE)
                , _allAcceptIOData(MAX_POST_ACCEPT_COUNT)
                , _freeSocketPool(FREE_SOCKET_POOL_RESERVE_SIZE) {
                _workerThreads.resize(0);
                _allAcceptIOData.resize(0);
                _freeSocketPool.resize(0);

                _ip[0] = '\0';

                _allocateCtx = []()->_ClientContext<_ALLOC> * {
                    return new (std::nothrow) _ClientContext<_ALLOC>;
                };
                _deallocateCtx = [](_impl::_ClientContext<_ALLOC> *ctx) {
                    delete ctx;
                };
            }

            ~_ServerFramework() {
            }

            bool startup(const char *ip, uint16_t port, const std::function<void (_ClientContext<_ALLOC> *ctx)> &onAccept,
                const  std::function<void (_ClientContext<_ALLOC> *ctx)> &onDisconnect) {

                _onAccept = onAccept;
                _onDisconnect = onDisconnect;

                _shouldQuit = false;

                // 完成端口
                _ioCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
                if (_ioCompletionPort == NULL) {
                    return false;
                }

                SYSTEM_INFO systemInfo;
                GetSystemInfo(&systemInfo);
                DWORD workerThreadCnt = systemInfo.dwNumberOfProcessors * 2 + 2;
                LOG_DEBUG("systemInfo.dwNumberOfProcessors = %u, workerThreadCnt = %u", systemInfo.dwNumberOfProcessors, workerThreadCnt);

                TRY_BLOCK_BEGIN
                _workerThreads.reserve(workerThreadCnt);

                // 工作线程
                while (workerThreadCnt-- > 0) {
                    std::thread *t = new (std::nothrow) std::thread(std::bind(&_ServerFramework<_ALLOC>::_WorketThreadProc, this));
                    if (t != nullptr) {
                        _workerThreads.push_back(t);
                    } else {
                        LOG_ERROR("new std::thread out of memory!");
                    }
                }
                CATCH_EXCEPTIONS
                return false;
                CATCH_BLOCK_END

                _listenSocket = ::WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
                if (_listenSocket == INVALID_SOCKET) {
                    _shouldQuit = true;
                    return false;
                }

                _port = port;
                struct sockaddr_in serverAddr = { 0 };
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = ::htons(_port);
                if (ip != nullptr) {
                    strncpy(_ip, ip, 16);
                    serverAddr.sin_addr.s_addr = ::inet_addr(ip);
                }

                if (::bind(_listenSocket, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                    _shouldQuit = true;
                    return false;
                }

                if (::listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
                    _shouldQuit = true;
                    return false;
                }

                _clientCount = 0;

                return _BeginAccept();
            }

            void shutdown() {
                _shouldQuit = true;
                if (_listenSocket != INVALID_SOCKET) {
                    ::closesocket(_listenSocket);
                    _listenSocket = INVALID_SOCKET;
                }

                // 结束所有工作线程
                for (size_t i = _workerThreads.size(); i > 0; --i) {
                    ::PostQueuedCompletionStatus(_ioCompletionPort, 0, (ULONG_PTR)nullptr, nullptr);
                }

                std::for_each(_workerThreads.begin(), _workerThreads.end(), [](std::thread *t) {
                    t->join();
                    delete t;
                });
                _workerThreads.clear();

                // 释放所有ClientContext.
                std::for_each(_clientList.begin(), _clientList.end(), _deallocateCtx);
                _clientList.clear();

                // 释放所有AcceptIOData.
                std::for_each(_allAcceptIOData.begin(), _allAcceptIOData.end(), [](_ACCEPT_OVERLAPPED<_ALLOC> *ioData) { delete ioData; });
                _allAcceptIOData.clear();

                ::CloseHandle(_ioCompletionPort);
                _ioCompletionPort = NULL;

                // 关闭所有socket
                std::for_each(_freeSocketPool.begin(), _freeSocketPool.end(), &::closesocket);
                _freeSocketPool.clear();
            }

        public:
            static inline bool initialize() {
                WSADATA data;
                WORD ver = MAKEWORD(2, 2);
                int ret = ::WSAStartup(ver, &data);
                if (ret != 0) {
                    LOG_ERROR("WSAStartup failed: last error %d", ::WSAGetLastError());
                    return false;
                }
                return true;
            }

            static inline bool uninitialize() {
                return ::WSACleanup() == 0;
            }

            inline const char *getIp() const { return _ip; }
            inline uint16_t getPort() const { return _port; }
            inline size_t getClientCount() const { return _clientCount; }

        private:
            bool _BeginAccept() {
                // 将listenSocket关联到完成端口上
                ::CreateIoCompletionPort((HANDLE)_listenSocket, _ioCompletionPort, 0, 0);

                if (!_GetFunctionPointers()) {
                    _shouldQuit = true;
                    return false;
                }

                // 投递AcceptEx.
                for (int i = 0; i < MAX_POST_ACCEPT_COUNT; ++i) {
                    _ACCEPT_OVERLAPPED<_ALLOC> *ioData = new (std::nothrow) _ACCEPT_OVERLAPPED<_ALLOC>;
                    if (ioData == nullptr) {
                        LOG_ERROR("new IODATA out of memory!");
                        continue;
                    }

                    if (_PostAccept(ioData)) {
                        _allAcceptIOData.push_back(ioData);
                        continue;
                    }

                    delete ioData;
                }

                return true;
            }

            bool _GetFunctionPointers() {
                GUID guidAcceptEx = WSAID_ACCEPTEX;
                DWORD bytes = 0;
                if (::WSAIoctl(_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &guidAcceptEx, sizeof(GUID),
                    &_acceptEx, sizeof(LPFN_ACCEPTEX),
                    &bytes, nullptr, nullptr) == SOCKET_ERROR) {
                    return false;
                }

                GUID guidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
                if (::WSAIoctl(_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &guidGetAcceptExSockAddrs, sizeof(GUID),
                    &_getAcceptExSockAddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
                    &bytes, nullptr, nullptr) == SOCKET_ERROR) {
                    return false;
                }

                GUID guidDisconnectEx = WSAID_DISCONNECTEX;
                if (::WSAIoctl(_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &guidDisconnectEx, sizeof(GUID),
                    &_disconnectEx, sizeof(LPFN_DISCONNECTEX),
                    &bytes, nullptr, nullptr) == SOCKET_ERROR) {
                    return false;
                }

                return true;
            }

            bool _DoAccept(_ACCEPT_OVERLAPPED<_ALLOC> *ioData) {
                SOCKET clientSocket = ioData->clientSocket;

                struct sockaddr_in *remoteAddr = nullptr;
                struct sockaddr_in *localAddr = nullptr;
                int remoteLen = sizeof(struct sockaddr_in);
                int localLen = sizeof(struct sockaddr_in);

                _getAcceptExSockAddrs(ioData->buf, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
                    (struct sockaddr **)&localAddr, &localLen, (struct sockaddr **)&remoteAddr, &remoteLen);

                const char *ip = ::inet_ntoa(remoteAddr->sin_addr);
                uint16_t port = ::ntohs(remoteAddr->sin_port);

                LOG_DEBUG("remote address %s %hu", ::inet_ntoa(remoteAddr->sin_addr), ::ntohs(remoteAddr->sin_port));
                LOG_DEBUG("local address %s %hu", ::inet_ntoa(localAddr->sin_addr), ::ntohs(localAddr->sin_port));

                _ClientContext<_ALLOC> *ctx = nullptr;
                TRY_BLOCK_BEGIN
                ctx = _allocateCtx();
                CATCH_EXCEPTIONS
                _RecycleSocket(clientSocket);
                return false;
                CATCH_BLOCK_END
                if (ctx == nullptr) {
                    LOG_ERROR("new context out of memory!");
                    _RecycleSocket(clientSocket);
                    return false;
                } else {
                    _clientMutex.lock();
                    TRY_BLOCK_BEGIN
                    _clientList.push_front(nullptr);  // 先用一个nullptr占位
                    list<_ClientContext<_ALLOC> *>::iterator it = _clientList.begin();  // 取得迭代器
                    ++_clientCount;
                    LOG_DEBUG("client count %lu", _clientCount);
                    _clientMutex.unlock();

                    ctx->_socket = clientSocket;
                    strncpy(ctx->_ip, ip, 16);
                    ctx->_port = port;
                    ctx->_iterator = it;
                    *it = ctx;  // 替换之前的占位符

                    // 将clientSocket关联到完成端口
                    if (::CreateIoCompletionPort((HANDLE)clientSocket, _ioCompletionPort, (ULONG_PTR)ctx, 0) != NULL) {
                        //if (ctx->postRecv() == _ClientContext<_ALLOC>::POST_RESULT::SUCCESS) {
                        _onAccept(ctx);
                            LOG_DEBUG("%16s:%5hu connected", ip, port);
                        //} else {
                        //    LOG_DEBUG("%16s:%5hu post recv failed", ip, port);
                        //}
                    }
                    CATCH_EXCEPTIONS
                    _clientMutex.unlock();
                    _RecycleSocket(clientSocket);
                    return false;
                    CATCH_BLOCK_END
                }
                return _PostAccept(ioData);
            }

            bool _PostAccept(_ACCEPT_OVERLAPPED<_ALLOC> *ioData) {
                return _PostAccept(ioData, [](_ServerFramework<_ALLOC> *thiz, _ClientContext<_ALLOC> *ctx, _BASE_OVERLAPPED<_ALLOC> *ol, size_t bytesTransfered) {
                    thiz->_DoAccept((_ACCEPT_OVERLAPPED<_ALLOC> *)ol);
                });
            }

            bool _PostAccept(_ACCEPT_OVERLAPPED<_ALLOC> *ioData, typename _BASE_OVERLAPPED<_ALLOC>::Operation op) {
                SOCKET clientSocket = INVALID_SOCKET;

                _poolMutex.lock();
                if (_freeSocketPool.empty()) {
                    _poolMutex.unlock();
                    clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);  // 创建一个新的
                } else {
                    clientSocket = _freeSocketPool.back();  // 重用
                    _freeSocketPool.pop_back();
                    _poolMutex.unlock();
                }

                if (clientSocket == INVALID_SOCKET) {
                    return false;
                }

                memset(ioData, 0, sizeof(OVERLAPPED));
                ioData->type = _OPERATION_TYPE::ACCEPT_POSTED;
                ioData->clientSocket = clientSocket;
                ioData->op = op;
                DWORD bytes = 0;

                // 为了让AcceptEx一旦有连接后立即返回，要将第四个参数设置为0
                BOOL ret = _acceptEx(_listenSocket, clientSocket, ioData->buf, 0,
                    sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16,
                    &bytes, (LPOVERLAPPED)ioData);

                if (!ret && ::WSAGetLastError() != WSA_IO_PENDING) {
                    return false;
                }
                return true;
            }

            inline void _RemoveExceptionalConnection(_ClientContext<_ALLOC> *ctx) {
                LOG_DEBUG("%16s:%5hu disconnected", ctx->_ip, ctx->_port);
                _onDisconnect(ctx);

                SOCKET s = ctx->_socket;

                _clientMutex.lock();
                ctx->_socket = INVALID_SOCKET;
                _clientList.erase(ctx->_iterator);  // 从链表中移除ClientContext
                _deallocateCtx(ctx);
                --_clientCount;
                LOG_DEBUG("client count %lu", _clientCount);
                _clientMutex.unlock();

                _RecycleSocket(s);
            }

            void _WorketThreadProc() {
                LPOVERLAPPED overlapped = nullptr;
                ULONG_PTR completionKey = 0;
                DWORD bytesTransfered = 0;

                while (!_shouldQuit) {
                    BOOL ret = ::GetQueuedCompletionStatus(_ioCompletionPort, &bytesTransfered, &completionKey, &overlapped, INFINITE);

                    _BASE_OVERLAPPED<_ALLOC> *ioData = (_BASE_OVERLAPPED<_ALLOC> *)overlapped;
                    CONTINUE_IF(ioData == nullptr);

                    if (!ret) {
                        DWORD lastError = ::GetLastError();
                        if (lastError == ERROR_NETNAME_DELETED || lastError == ERROR_PORT_UNREACHABLE) {
                            if (ioData->type == _OPERATION_TYPE::ACCEPT_POSTED) {
                                // 这里应该要重新补充一个AcceptEx请求
                                _PostAccept((_ACCEPT_OVERLAPPED<_ALLOC> *)completionKey);
                            } else {
                                _ClientContext<_ALLOC> *ctx = (_ClientContext<_ALLOC> *)completionKey;
                                if (ctx != nullptr) {
                                    _RemoveExceptionalConnection(ctx);
                                }
                            }
                            continue;
                        }

                        CONTINUE_IF(lastError == WAIT_TIMEOUT);
                        CONTINUE_IF(overlapped != nullptr);
                        break;
                    }

                    _ClientContext<_ALLOC> *ctx = (_ClientContext<_ALLOC> *)completionKey;
                    if (ctx == nullptr) {
                        if (ioData->type == _OPERATION_TYPE::ACCEPT_POSTED) {
                            ioData->op(this, ctx, ioData, bytesTransfered);
                        }
                        continue;
                    }

                    switch (ioData->type) {
                    case _OPERATION_TYPE::RECV_POSTED:
                        if (bytesTransfered == 0) {
                            _RemoveExceptionalConnection(ctx);
                        }
                        ioData->op(this, ctx, ioData, bytesTransfered);
                        break;
                    case _OPERATION_TYPE::SEND_POSTED:
                        ioData->op(this, ctx, ioData, bytesTransfered);
                        break;
                    case _OPERATION_TYPE::NULL_POSTED:
                        break;
                    default:
                        break;
                    }
                }
            }

            inline void _RecycleSocket(SOCKET s) {
                _disconnectEx(s, nullptr, TF_REUSE_SOCKET, 0);
                _poolMutex.lock();
                TRY_BLOCK_BEGIN
                _freeSocketPool.push_back(s);
                CATCH_EXCEPTIONS
                ::closesocket(s);
                CATCH_BLOCK_END
                _poolMutex.unlock();
            }
        };
    }  // end of namespace _impl

    template <class _T> using ClientContext = _impl::_ClientContext<std::allocator>;
    template <class _T> using ServerFramework = _impl::_ServerFramework<std::allocator>;

}  // endof namespace iocp

#pragma warning(pop)

#endif
