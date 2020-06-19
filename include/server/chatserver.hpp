#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;

class ChatServer{

public:
    // 初始化聊天服务对象
    ChatServer(EventLoop *loop,
               const InetAddress& addr,
               const string& nameArg);
    // 启动服务
    void start();
    // 客户端连接和断开的回调函数
    void onConnection(const TcpConnectionPtr&);
    // 读写事件的相关回调函数
    void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);

private:
    TcpServer _server;  // 组合的muduo库，实现服务端的功能
    EventLoop *_loop;   // 指向事件循环
};



#endif //CHATSERVER_H