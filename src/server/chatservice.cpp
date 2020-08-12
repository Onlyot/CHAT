#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"
#include <muduo/base/Logging.h>
#include <map>
using namespace muduo;

// Test
#include <iostream>

// 获取单例对象指针的函数接口
ChatService* ChatService::instance()
{
    static ChatService _service;
    return &_service;
}

// 根据信息类型绑定回调函数
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap[LOGIN_MSG] = bind(&ChatService::login, this, _1, _2, _3);
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap[REG_MSG] = bind(&ChatService::reg, this, _1, _2, _3);
    _msgHandlerMap[ONE_CHAT_MSG] = bind(&ChatService::oneChat, this, _1, _2, _3);
    _msgHandlerMap[ADD_FRIEND_MSG] = bind(&ChatService::addFriend, this, _1, _2, _3);

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

 // 服务器异常退出，业务重置方法
void ChatService::reset()
{
    _userModel.resetState();
}


// 处理登录业务 id name passwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPwd() == pwd){
        // 判断是否重复登录
        if(user.getState() == "online"){
            // 重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "the user had online";

            conn->send(response.dump());
        }
        else{
            
            {
                // 登录成功并保存连接（要考虑线程安全）
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
                // 上述设计的优点使得无须额外增加解锁的操作，让系统一次释放资源
            }

            // id用户登录成功后，订阅消息队列
            _redis.subscribe(id);

            // 更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);

            // 登录成功并更新用户状态信息
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if(!vec.empty()){
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(user.getId());
            }

            // 查询用户的好友信息并返回
            vector<User> userVec = _friendModel.query(user.getId());
            if(!userVec.empty()){
                vector<string> vec2;
                for(User &user :userVec){
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty()){
                
                vector<string> groupV;
                for(Group &group : groupuserVec){
                    // 遍历每一个群组
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    vector<string> userV; 
                    for(GroupUser &user : group.getUsers()){
                        // 遍历每个群组中的每个成员
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }else{
        // 账号或者密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "the id or password is error";
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}


// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];
    User user;
    user.setName(name);
    user.setPwd(password);
    bool state = _userModel.insert(user);
    if(state){
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }else{
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 获取消息对应的处理器
msgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()){
        return [=](const TcpConnectionPtr& conn, json &js, Timestamp time){
            LOG_ERROR << "msgid: " << msgid << " cannot find handler!";
        };
    }
    return _msgHandlerMap[msgid];
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    // 确保线程安全
    {
        // 删除保存登录的用户信息
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); it++){
            if(it->second == conn){
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 取消用户的订阅
    _redis.unsubscribe(user.getId());

    if(user.getId() != -1){
        // 更新用户的状态信息
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &con, json &js, Timestamp time)
{
    // Test
    std::cout << js.dump() << std::endl;

    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){
            // 目标对象在线，服务端转发信息
            it->second->send(js.dump());

            return;
        }
    }

    // 判断用户是否在其他的服务器上
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.pushlish(toid, js.dump());
        return;
    }

    // toid目标对象不在线，转发消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    
    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group)){
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec){
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end()){
            // 转发消息
            it->second->send(js.dump());
        }else{

            User user = _userModel.query(userid);
            if(user.getState() == "online")
            {
                _redis.pushlish(id, js.dump());
            }
            else
            {
                // 存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
            

        }
    }
}

// 从 redis 消息队列中获取订阅的信息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    _offlineMsgModel.insert(userid, msg);
}