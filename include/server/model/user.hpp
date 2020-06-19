#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// User表的ORM类
class User{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        m_id = id;
        m_name = name;
        m_password = pwd;
        m_state = state;
    }

    void setId(int id){m_id = id;}
    void setName(const string& name){m_name = name;}
    void setPwd(const string& pwd){m_password = pwd;}
    void setState(const string& state){m_state = state;}

    int getId() const {return m_id;}
    string getName() const {return m_name;}
    string getPwd() const {return m_password;}
    string getState() const {return m_state;}

private:
    int m_id;
    string m_name;
    string m_password;
    string m_state;
};

#endif