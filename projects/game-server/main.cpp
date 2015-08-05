#include "../common-test/IOService.hpp"
#include "GameServer.h"
#include "../common-test/TimerEngine.h"

enum E1 { value };
enum class E2 { value };

#include <iostream>

int main(int argc, char *argv[]) {
    system("chcp 65001");

    jw::TimerEngine::getInstance();
    jw::IOService<Server> s(8899);
    (void)s;
    getchar();
    jw::TimerEngine::destroyInstance();
    return 0;
}

#include "../common-test/LogUtil.cpp"
#include "../common-test/TimerEngine.cpp"
