#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务对象
ChatServer::ChatServer(EventLoop *loop,
            const InetAddress& addr,
            const string& nameArg)
    : _server(loop, addr, nameArg), _loop(loop)
{
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}
// 启动服务
void ChatServer::start()
{
    _server.start();
}
// 客户端连接和断开的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    // 连接失败
    if(!conn->connected()){

        //处理异常关闭
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 读写事件的相关回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);
    // 解耦网络模块和业务模块代码

    // 通过 js["msgid"] 获取 =》 void callback(conn, js, time)
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());

    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}
