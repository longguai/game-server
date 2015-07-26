#define _CRT_SECURE_NO_WARNINGS

#include <utility>
#include <thread>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define ASIO_STANDALONE
#include "../../support/asio-1.10.4/asio.hpp"

namespace echo {

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(asio::ip::tcp::socket &&socket) : _socket(std::move(socket)) {
        }

        ~Session() {
            printf("Session::~Session");
        }

        void start() {
            _doRead();
        }

    private:
        void _doRead() {
            auto thiz = shared_from_this();
            _socket.async_read_some(asio::buffer(_data, max_length),
                [this, thiz](std::error_code ec, size_t length) {
                if (!ec) {
                    _doWrite(length);
                }
            });
        }

        void _doWrite(size_t length) {
            auto thiz = shared_from_this();
            asio::async_write(_socket, asio::buffer(_data, length),
                [this, thiz](std::error_code ec, size_t length) {
                if (!ec) {
                    _doRead();
                }
            });
        }

        asio::ip::tcp::socket _socket;

        enum { max_length = 1024 };
        char _data[max_length];
    };

    class Server {
    public:
        Server(asio::io_service &service, unsigned short port)
            : _acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
            _socket(service) {
            _doAccept();
        }

    private:
        void _doAccept() {
            _acceptor.async_accept(_socket, [this](std::error_code ec) {
                if (!ec) {
                    std::make_shared<Session>(std::move(_socket))->start();
                }
                _doAccept();
            });
        }

        asio::ip::tcp::acceptor _acceptor;
        asio::ip::tcp::socket _socket;
    };

}

#include <deque>
#include <unordered_set>
#include <mutex>

namespace chat {

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(asio::ip::tcp::socket &&socket, std::function<void (const std::shared_ptr<Session> &, const char *, size_t)> &&callback)
            : _socket(std::move(socket))
            , _callback(callback) {
        }

        ~Session() {
            printf("Session::~Session");
        }

        void start() {
            _doRead();
        }

        void deliver(const char *data, size_t length) {
            bool empty = _writeQueue.empty();
            _writeQueue.push_back(asio::buffer(data, length));
            if (empty) {
                asio::async_write(_socket, _writeQueue.front(), std::bind(&Session::_doWrite, this, std::placeholders::_1, std::placeholders::_2));
            }
        }

    private:
        void _doRead() {
            auto thiz = shared_from_this();
            _socket.async_read_some(asio::buffer(_readData, max_length), [this, thiz](std::error_code ec, size_t length) {
                if (!ec) {
                    _callback(thiz, _readData, length);
                    _doRead();
                } else {
                    _callback(thiz, nullptr, 0);
                }
            });
        }

        void _doWrite(std::error_code ec, size_t length) {
            if (!ec) {
                _writeQueue.pop_front();
                if (!_writeQueue.empty()) {
                    asio::async_write(_socket, _writeQueue.front(), std::bind(&Session::_doWrite, this, std::placeholders::_1, std::placeholders::_2));
                }
            } else {
                _callback(shared_from_this(), nullptr, 0);
            }
        }

        asio::ip::tcp::socket _socket;
        enum { max_length = 1024 };
        char _readData[max_length];

        std::deque<asio::const_buffers_1> _writeQueue;

        std::function<void (const std::shared_ptr<Session> &, const char *, size_t)> _callback;
    };

    class Server {
    public:
        Server(asio::io_service &service, unsigned short port)
            : _acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
            for (size_t i = 0; i < max_accept; ++i) {
                _socket[i] = std::make_shared<asio::ip::tcp::socket>(service);
            }
            _doAccept();
        }

        ~Server() {
            printf("Server::~Server");
        }
    private:
        void _doAccept() {
            for (size_t i = 0; i < max_accept; ++i) {
                _acceptor.async_accept(*_socket[i], std::bind(&Server::_acceptCallback, this, i, std::placeholders::_1));
            }
        }

        void _acceptCallback(size_t index, std::error_code ec) {
            if (!ec) {
                std::shared_ptr<Session> s = std::make_shared<Session>(std::move(*_socket[index]),
                    std::bind(&Server::_sessionCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _mutex.lock();
                _sessions.insert(s);
                _mutex.unlock();
                s->start();
            }
            _acceptor.async_accept(*_socket[index], std::bind(&Server::_acceptCallback, this, index, std::placeholders::_1));
        }

        void _sessionCallback(const std::shared_ptr<Session> &s, const char *data, size_t length) {
            if (data != nullptr) {
                _mutex.lock();
                std::for_each(_sessions.begin(), _sessions.end(), [data, length](const std::shared_ptr<Session> &s) {
                    s->deliver(data, length);
                });
                _mutex.unlock();
            } else {
                _mutex.lock();
                _sessions.erase(s);
                _mutex.unlock();
            }
        }

        asio::ip::tcp::acceptor _acceptor;

        enum { max_accept = 128 };
        std::shared_ptr<asio::ip::tcp::socket> _socket[max_accept];

        std::unordered_set<std::shared_ptr<Session> > _sessions;
        std::mutex _mutex;
    };

}

int main(int argc, char *argv[])
{
    try {
        asio::io_service service;
        //echo::Server s(service, 8899);
        chat::Server s(service, 8899);
        asio::io_service::work work(service);

        SYSTEM_INFO systemInfo;
        ::GetSystemInfo(&systemInfo);
        DWORD workerThreadCnt = systemInfo.dwNumberOfProcessors * 2 + 2;
        std::vector<std::thread *> workerThreads;
        for (DWORD i = 0; i < workerThreadCnt; ++i) {
            workerThreads.push_back(new (std::nothrow) std::thread([&service](){
                service.run();
            }));
        }

        while (scanf("%*c") != EOF) {
            continue;
        }
        service.stop();

        for (DWORD i = 0; i < workerThreadCnt; ++i) {
            if (workerThreads[i]->joinable()) {
                workerThreads[i]->join();
            }
            delete workerThreads[i];
        }
    }
    catch (std::exception &e) {
        printf("%s\n", e.what());
    }

    return 0;
}
