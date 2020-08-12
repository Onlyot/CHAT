#include <iostream>
#include <signal.h>
#include "chatserver.hpp"
#include "chatservice.hpp"
using namespace std;

void resetHandler(int sig)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char* argv[]){

    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    signal(SIGINT, resetHandler);

    EventLoop loop;

    const char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}