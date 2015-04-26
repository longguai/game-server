#if (defined _DEBUG) || (defined DEBUG)
#   define CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#endif

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "src/iocp-exception.h"
#include "src/iocp-core.h"
#include <windows.h>

static char buf[1024];

void sendCallback(iocp::ServerFramework<void> *server, iocp::ClientContext<void> *ctx, iocp::_impl::_BASE_OVERLAPPED<std::allocator> *thiz, size_t bytesTransfered);

void recvCallback(iocp::ServerFramework<void> *server, iocp::ClientContext<void> *ctx, iocp::_impl::_BASE_OVERLAPPED<std::allocator> *thiz, size_t bytesTransfered)
{
    ctx->asyncSend(buf, bytesTransfered, sendCallback);
}

void sendCallback(iocp::ServerFramework<void> *server, iocp::ClientContext<void> *ctx, iocp::_impl::_BASE_OVERLAPPED<std::allocator> *thiz, size_t bytesTransfered)
{
    ctx->asyncRecv(buf, 1024, recvCallback);
}

int main() {

#if (defined _DEBUG) || (defined DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(1217);
#endif

    iocp::ServerFramework<void>::initialize();

    try {
        iocp::ServerFramework<void> server;

        server.startup(nullptr, 8899, [&server](iocp::ClientContext<void> *ctx) {
            ctx->asyncRecv(buf, 1024, recvCallback);
        }, [](iocp::ClientContext<void> *ctx) {
            printf("disconnect %s : %hu\n", ctx->getIp(), ctx->getPort());
        });

        while (scanf("%*c") != EOF)
            continue;
        server.shutdown();
    }
    catch (std::exception &e) {
        printf("%s\n", e.what());
    }

    iocp::ServerFramework<void>::uninitialize();

    return 0;
}
