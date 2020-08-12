#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

public:
    bool connect();

    bool pushlish(int channel, string message);
    
    bool subscribe(int channel);
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();

    // 初始向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    redisContext *_publish_context;
    redisContext *_subcribe_context;
    // 收到订阅消息后的回调函数
    function<void(int, string)> _notify_message_handler;
};


#endif