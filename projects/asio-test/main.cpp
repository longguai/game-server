//#include <windows.h>
#include <memory>
#include <utility>

#define ASIO_STANDALONE
#include "../../support/asio-1.10.4/asio.hpp"

class session : public std::enable_shared_from_this<session>
{
public:
    session(asio::ip::tcp::socket socket) : _socket(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        auto self(shared_from_this());
        _socket.async_read_some(asio::buffer(_data, max_length),
            [this, self](std::error_code ec, std::size_t length) {
            if (!ec)
            {
                do_write(length);
            }
        });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        asio::async_write(_socket, asio::buffer(_data, length),
            [this, self](std::error_code ec, std::size_t length) {
            if (!ec)
            {
                do_read();
            }
        });
    }

    asio::ip::tcp::socket _socket;
    enum { max_length = 1024 };
    char _data[max_length];
};

class server
{
public:
    server(asio::io_service& io_service, short port)
        : _acceptor(io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
        _socket(io_service)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        _acceptor.async_accept(_socket, [this](std::error_code ec) {
            if (!ec)
            {
                std::make_shared<session>(std::move(_socket))->start();
            }
            do_accept();
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
        server s(ios, 8899);
        ios.run();
    }
    catch (std::exception &e) {
        printf("%s\n", e.what());
    }

    return 0;
}
