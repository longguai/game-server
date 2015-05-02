#include <utility>

#define ASIO_STANDALONE
#include "../../support/asio-1.10.4/asio.hpp"

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
            [this](std::error_code ec, std::size_t length) {
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
    Server(asio::io_service& io_service, short port)
        : _acceptor(io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
        _socket(io_service)
    {
        _doAccept();
    }

private:
    void _doAccept()
    {
        _acceptor.async_accept(_socket, [this](std::error_code ec) {
            if (!ec)
            {
                (new Session(std::move(_socket)))->start();
            }
            _doAccept();
        });
    }

    asio::ip::tcp::acceptor _acceptor;
    asio::ip::tcp::socket _socket;
};


int main(int argc, const char *argv[])
{
    printf("test asio");

    try {
        asio::io_service ios;
        Server s(ios, 8899);
        ios.run();
    }
    catch (std::exception &e) {
        printf("%s\n", e.what());
    }

    return 0;
}
