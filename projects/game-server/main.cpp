#define _CRT_SECURE_NO_WARNINGS

#include <utility>
#include <thread>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define ASIO_STANDALONE
#include "../../support/asio-1.10.4/asio.hpp"

namespace echo {

    class Session
    {
    public:
        Session(asio::ip::tcp::socket &&socket) : _socket(std::move(socket))
        {
        }

        void start()
        {
            _doRead();
        }

    private:
        void _doRead()
        {
            _socket.async_read_some(asio::buffer(_data, max_length),
                [this](std::error_code ec, size_t length) {
                if (!ec)
                {
                    _doWrite(length);
                }
                else
                {
                    delete this;
                }
            });
        }

        void _doWrite(size_t length)
        {
            asio::async_write(_socket, asio::buffer(_data, length),
                [this](std::error_code ec, size_t length) {
                if (!ec)
                {
                    _doRead();
                }
                else
                {
                    delete this;
                }
            });
        }

        asio::ip::tcp::socket _socket;

        enum { max_length = 1024 };
        char _data[max_length];
    };

    class Server
    {
    public:
        Server(asio::io_service &service, unsigned short port)
            : _acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
            _socket(service)
        {
            _doAccept();
        }

    private:
        void _doAccept()
        {
            _acceptor.async_accept(_socket, [this](std::error_code ec) {
                if (!ec)
                {
                    Session *s = new (std::nothrow) Session(std::move(_socket));
                    if (s != nullptr)
                    {
                        s->start();
                    }
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

    class Session
    {
    public:
        Session(asio::ip::tcp::socket &&socket, std::function<void (Session *, std::error_code, const char *, size_t)> &&callback)
            : _socket(std::move(socket))
            , _callback(callback)
        {
        }

        void start()
        {
            _doRead();
        }

        void deliver(const char *data, size_t length)
        {
            bool empty = _writeQueue.empty();
            _writeQueue.push_back(asio::buffer(data, length));
            if (empty)
            {
                asio::async_write(_socket, _writeQueue.front(), std::bind(&Session::_doWrite, this, std::placeholders::_1, std::placeholders::_2));
            }
        }

    private:
        void _doRead()
        {
            _socket.async_read_some(asio::buffer(_readData, max_length), [this](std::error_code ec, size_t length) {
                _callback(this, ec, _readData, length);
                if (!ec)
                {
                    _doRead();
                }
            });
        }

        void _doWrite(std::error_code ec, size_t length)
        {
            if (!ec)
            {
                _writeQueue.pop_front();
                if (!_writeQueue.empty())
                {
                    asio::async_write(_socket, _writeQueue.front(), std::bind(&Session::_doWrite, this, std::placeholders::_1, std::placeholders::_2));
                }
            }
            else
            {
                _callback(this, ec, nullptr, 0);
            }
        }

        asio::ip::tcp::socket _socket;
        enum { max_length = 1024 };
        char _readData[max_length];

        std::deque<asio::const_buffers_1> _writeQueue;

        std::function<void (Session *, std::error_code, const char *, size_t)> _callback;
    };

    class Server
    {
    public:
        Server(asio::io_service &service, unsigned short port)
            : _acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
            _socket(service)
        {
            _doAccept();
        }

    private:
        void _doAccept()
        {
            _acceptor.async_accept(_socket, [this](std::error_code ec) {
                if (!ec)
                {
                    Session *s = new (std::nothrow) Session(std::move(_socket), [this](Session *s, std::error_code ec, const char *data, size_t length) {
                        if (!ec)
                        {
                            _mutex.lock();
                            std::for_each(_sessions.begin(), _sessions.end(), [data, length](Session *s) {
                                s->deliver(data, length);
                            });
                            _mutex.unlock();
                        }
                        else
                        {
                            _mutex.lock();
                            _sessions.erase(s);
                            _mutex.unlock();
                            delete s;
                        }
                    });
                    if (s != nullptr)
                    {
                        _mutex.lock();
                        _sessions.insert(s);
                        _mutex.unlock();
                        s->start();
                    }
                }
                _doAccept();
            });
        }

        asio::ip::tcp::acceptor _acceptor;
        asio::ip::tcp::socket _socket;

        std::unordered_set<Session *> _sessions;
        std::mutex _mutex;
    };

}

int main(int argc, char *argv[])
{
    try {
        asio::io_service service;
        //echo::Server s(service, 8899);
        chat::Server s(service, 8899);

        SYSTEM_INFO systemInfo;
        ::GetSystemInfo(&systemInfo);
        DWORD workerThreadCnt = systemInfo.dwNumberOfProcessors * 2 + 2;
        std::vector<std::thread *> workerThreads;
        for (DWORD i = 0; i < workerThreadCnt; ++i)
        {
            workerThreads.push_back(new (std::nothrow) std::thread([&service](){
                service.run();
            }));
        }

        while (scanf("%*c") != EOF)
        {
            continue;
        }
        service.stop();

        for (DWORD i = 0; i < workerThreadCnt; ++i)
        {
            workerThreads[i]->join();
            delete workerThreads[i];
        }
    }
    catch (std::exception &e) {
        printf("%s\n", e.what());
    }

    return 0;
}
