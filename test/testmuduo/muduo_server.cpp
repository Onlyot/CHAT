#include <iostream>
#include <functional>
#include <string>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;

class ChatServer{
public:
    ChatServer(EventLoop* loop, const InetAddress& addr) : _server(loop, addr, "Chatserver"){
        _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
        _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

        _server.setThreadNum(8);
    }

    void start(){
        _server.start();
    }

private:
    void onConnection(const TcpConnectionPtr& con){
        if(con->connected()){
            cout << con->peerAddress().toIpPort() << " -> "
             << con->localAddress().toIpPort() << "state:onstate" << endl;
        }else{
            cout << con->peerAddress().toIpPort() << " -> "
             << con->localAddress().toIpPort() << "state:offstate" << endl;
             con->shutdown(); // close(fd)
        }
    }

    void onMessage(const TcpConnectionPtr& con, Buffer* buffer, Timestamp time){
        string buf = buffer->retrieveAllAsString();
        cout << "recv data: " << buf;
        cout << "time: " << time.toString() << endl;
    }

private:
    TcpServer _server;
};

int main(){

    EventLoop loop;
    const InetAddress addr("127.0.0.1", 9190);
    ChatServer server(&loop, addr);

    server.start();
    
    loop.loop();

    return 0;
}