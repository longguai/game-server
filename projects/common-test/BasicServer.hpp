#ifndef _BASIC_SERVER_HPP_
#define _BASIC_SERVER_HPP_

#include "asio_header.hpp"
#include "DebugConfig.h"
#include <stddef.h>
#include <memory>
#include <functional>

namespace jw {
    struct ProxyExample {
        void acceptCallback(asio::ip::tcp::socket &&socket) {
        }
    };

    // _ServerProxy须要一个void acceptCallback(asio::ip::tcp::socket &&socket)作为成员函数
    // _MaxAccept为同时发起的Accept数量
    template <class _ServerProxy, size_t _MaxAccept>
    class BasicServer : public _ServerProxy {
    public:
        BasicServer<_ServerProxy, _MaxAccept>(const BasicServer<_ServerProxy, _MaxAccept> &) = delete;
        BasicServer<_ServerProxy, _MaxAccept> &operator=(const BasicServer<_ServerProxy, _MaxAccept> &) = delete;

        BasicServer<_ServerProxy, _MaxAccept>(asio::io_service &service, unsigned short port)
            : _acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
            for (size_t i = 0; i < _MaxAccept; ++i) {
                _sockets[i] = std::make_shared<asio::ip::tcp::socket>(service);
            }
            _doAccept();
        }

        ~BasicServer<_ServerProxy, _MaxAccept>() {
            LOG_INFO("BasicServer<_ServerProxy, _MaxAccept>::~BasicServer<_ServerProxy, _MaxAccept>");
        }

    private:
        void _doAccept() {
            // 发起accept操作
            for (size_t i = 0; i < _MaxAccept; ++i) {
                _acceptor.async_accept(*_sockets[i], std::bind(&BasicServer<_ServerProxy, _MaxAccept>::_acceptCallback, this, i, std::placeholders::_1));
            }
        }

        void _acceptCallback(size_t index, std::error_code ec) {
            if (!ec) {
                // accept成功
                _ServerProxy::acceptCallback(std::move(*_sockets[index]));
            }
            // 发起下一次accept操作
            _acceptor.async_accept(*_sockets[index], std::bind(&BasicServer<_ServerProxy, _MaxAccept>::_acceptCallback, this, index, std::placeholders::_1));
        }

        asio::ip::tcp::acceptor _acceptor;
        std::shared_ptr<asio::ip::tcp::socket> _sockets[_MaxAccept];
    };
}

#endif
