#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <muduo/net/TcpServer.h>
#include <mutex>
#include "groupmodel.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "json.hpp"
using namespace std;
using namespace muduo::net;
using namespace muduo;

using json = nlohmann::json;

using msgHandler = std::function<void (const TcpConnectionPtr&, json&, Timestamp)>;

class ChatService{

public:
    // 获取单例对象指针
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &con, json &js, Timestamp time);

    // 服务器异常退出，业务重置方法
    void reset();

    // 获取消息对应的处理器
    msgHandler getHandler(int msgid);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

private:
    ChatService();

private:
    // 存储消息id和其业务处理方法
    unordered_map<int, msgHandler> _msgHandlerMap;

    // 保存登录用户的连接(要考虑线程安全,因为用户的登录注销等)
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作对象
    UserModel _userModel;
    OfflineMessageModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
}; 


#endif 