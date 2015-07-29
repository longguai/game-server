#ifndef _IO_SERVICE_HPP_
#define _IO_SERVICE_HPP_

#include "asio_header.hpp"
#include "DebugConfig.h"
#include <vector>
#include <thread>

namespace jw {

    // _Server的构造函数必须为_Server(asio::io_service &service, unsigned port);
    template <class _Server>
    class IOService {
    private:
        // 构造顺序：先构造_service，再用它构造_server和_work
        asio::io_service _service;
        _Server _server;
        asio::io_service::work _work;
        std::vector<std::thread *> _workerThreads;

    public:
        IOService<_Server>(const IOService<_Server> &) = delete;
        IOService<_Server> &operator=(const IOService<_Server> &) = delete;

        IOService<_Server>(unsigned short port) try : _server(_service, port), _work(_service) {
            int hc = std::thread::hardware_concurrency();
            hc *= 2;
            hc += 2;
            while (hc-- > 0) {
                _workerThreads.push_back(new (std::nothrow) std::thread([this]() {
                    _service.run();
                }));
            }
        }
        catch (std::exception &e) {
            LOG_ERROR("%s", e.what());
        }

        ~IOService<_Server>() {
            _service.stop();

            while (_workerThreads.empty()) {
                std::thread *t = _workerThreads.back();
                if (t != nullptr) {
                    if (t->joinable()) {
                        t->join();
                    }
                    delete t;
                }
                _workerThreads.pop_back();
            }
        }
    };
}

#endif
