
#ifndef __MY_USER__
#define __MY_USER__

#include <string>
#include <unordered_map>
#include <mutex>
#include "Util.hpp" 

#define USER_FILE "user.dat"

namespace wzh {

class UserManager {
public:
    // 获取单例实例
    static UserManager& getInstance() 
    {
        static UserManager instance;
        return instance;
    }

    // 注册用户
    bool registerUser(const std::string& username, const std::string& password) 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (users_.count(username)) return false;
        users_[username] = password;
        saveUsers();
        return true;
    }

    // 登录验证
    bool loginUser(const std::string& username, const std::string& password) 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = users_.find(username);
        return it != users_.end() && it->second == password;
    }

private:
    UserManager() { loadUsers(); }
    ~UserManager() { }
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    void loadUsers() 
    {
        if (!FileUtil(USER_FILE).exits()) return;
        std::string content;
        if (!FileUtil(USER_FILE).getContent(&content)) return;
        Json::Value root;
        if (!JsonUtil::deSerialize(content, &root)) return;
        for (const auto& name : root.getMemberNames()) 
        {
            users_[name] = root[name].asString();
        }
    }

    void saveUsers() 
    {
        Json::Value root;
        for (const auto& kv : users_) root[kv.first] = kv.second;
        std::string out;
        if (!JsonUtil::serialize(root, &out)) return;
        FileUtil(USER_FILE).setContent(out);
    }

    std::unordered_map<std::string, std::string> users_;
    std::mutex mutex_;
};

} // namespace wzh

#endif // __MY_USER__