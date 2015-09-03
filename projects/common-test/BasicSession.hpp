#ifndef _BASIC_SESSION_HPP_
#define _BASIC_SESSION_HPP_

#include "asio_header.hpp"
#include "DebugConfig.h"
#include "QuickMutex.h"
#include <stddef.h>
#include <memory>
#include <functional>
#include <type_traits>
#include <utility>
#include <deque>

namespace jw {
    struct Nothing {};

    enum class SessionEvent {
        Send = 0,
        Recv
    };

    // _BufSize为write缓冲区的大小
    template <class _Extra, size_t _BufSize>
    class BasicSession : public _Extra, public std::enable_shared_from_this<BasicSession<_Extra, _BufSize> > {
    public:
        BasicSession<_Extra, _BufSize>(const BasicSession<_Extra, _BufSize> &) = delete;
        BasicSession<_Extra, _BufSize> &operator=(const BasicSession<_Extra, _BufSize> &) = delete;

        typedef std::shared_ptr<BasicSession<_Extra, _BufSize> > SessionPtr;
        typedef std::function<void (const SessionPtr &, SessionEvent, const char *, size_t)> SessionCallback;

        template <class _Callable>
        BasicSession<_Extra, _BufSize>(asio::ip::tcp::socket &&socket, _Callable &&sessionCallback)
            : _socket(std::move(socket)) {
            try {
                static_assert(std::is_convertible<_Callable, SessionCallback>::value, "");
                _sessionCallback = sessionCallback;

                // 保存远程和本地的IP、端口
                asio::ip::tcp::endpoint remote = _socket.remote_endpoint();
                asio::ip::tcp::endpoint local = _socket.local_endpoint();
                _remoteIP = remote.address().to_string();
                _remotePort = remote.port();
                _localIP = local.address().to_string();
                _localPort = local.port();
                LOG_INFO("remote %s:%hu local %s:%hu", _remoteIP.c_str(), _remotePort, _localIP.c_str(), _localPort);
            }
            catch (std::exception &e) {
                LOG_ERROR("%s", e.what());
            }
        }

        ~BasicSession<_Extra, _BufSize>() {
            LOG_DEBUG("BasicSession<_Extra, _BufSize>::~BasicSession<_Extra, _BufSize>");
        }

        inline void start() {
            _doRead();
        }

        const std::string &getRemoteIP() const { return _remoteIP; }
        unsigned short getRemotePort() const { return _remotePort; }
        const std::string &getLocalIP() const { return _localIP; }
        unsigned short getLocalPort() const { return _localPort; }

        void deliver(std::vector<char> &&buf) {
            std::lock_guard<jw::QuickMutex> g(_mutex);
            (void)g;

            bool empty = _writeQueue.empty();
            _writeQueue.push_back(std::move(buf));
            // push之前的发送队列为空，则须要发起write
            if (empty) {
                _doWrite();
            }
        }

        void deliver(const void *data, size_t length) {
            deliver(std::vector<char>((char *)data, (char *)data + length));
        }

        void deliver(const std::vector<char> &buf) {
            deliver(&buf.at(0), buf.size());
        }

        void deliver(const std::string &str) {
            deliver(str.c_str(), str.length());
        }

    private:
        void _doRead() {
            auto thiz = shared_from_this();
            _socket.async_read_some(asio::buffer(_readData, _BufSize), [this, thiz](std::error_code ec, size_t length) {
                if (!ec) {
                    _sessionCallback(thiz, SessionEvent::Recv, _readData, length);
                    // 接收成功，须要发起read
                    _doRead();
                }
                else {
                    _sessionCallback(thiz, SessionEvent::Recv, nullptr, 0);
                }
            });
        }

        inline void _doWrite() {
            asio::async_write(_socket, asio::buffer(&_writeQueue.front()[0], _writeQueue.front().size()),
                std::bind(&BasicSession<_Extra, _BufSize>::_writeCallback, this, std::placeholders::_1, std::placeholders::_2));
        }

        void _writeCallback(std::error_code ec, size_t length) {
            std::lock_guard<jw::QuickMutex> g(_mutex);
            (void)g;

            if (!ec) {
                _writeQueue.pop_front();
                // 发送队列不为空，则须要发起下一次write
                if (!_writeQueue.empty()) {
                    _doWrite();
                }
            }
            else {
                // 发送失败
                _sessionCallback(shared_from_this(), SessionEvent::Send, nullptr, 0);
            }
        }

        asio::ip::tcp::socket _socket;
        std::string _remoteIP;
        unsigned short _remotePort = 0;
        std::string _localIP;
        unsigned short _localPort = 0;
        char _readData[_BufSize];

        std::deque<std::vector<char> > _writeQueue;
        jw::QuickMutex _mutex;

        SessionCallback _sessionCallback;
    };

    typedef BasicSession<Nothing, 1024U> Session;
}

#endif
