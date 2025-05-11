
#ifndef __MY_DATA__
#define __MY_DATA__

#include <unordered_map>
#include <pthread.h>
#include "Util.hpp"
#include <queue>
#include "config.hpp"
#include <chrono>
#include <atomic>
#include <thread>

namespace wzh
{
  typedef struct BackupInfo
  {
    bool pack_flag;
    std::string real_path;
    std::string pack_path; 
    std::string url;
    time_t last_modTime;
    size_t file_size;

    bool newBackupInfo(const std::string &realpath)
    {
      FileUtil fu(realpath);
      if(fu.exits() == false)
      {
        std::cout << "new BackupInfo failed, file not exist\n";
        return false;
      }
      config *con = config::getInstance();
      std::string packdir = con->getPackDir();
      std::string packsuffix = con->getPackFileSuffix();
      std::string download_preffix = con->getDownloadPreffix();
      pack_flag = false;
      last_modTime = fu.lastModTime();
      file_size = fu.fileSize();
      real_path = realpath;
      pack_path = packdir + fu.fileName() + packsuffix;
      url = download_preffix + fu.fileName();
      return true;
    }
  }BackupInfo;
  
  class DataManager
  {
  public:
    DataManager() : _running(true), _changes(0)
    {
      _worker = std::thread(&DataManager::storageThread, this);
      _backup_file = config::getInstance()->getBackupFile();
      pthread_rwlock_init(&_rwlock, NULL);
      InitLoad();
    }

    ~DataManager()
    {
      pthread_rwlock_destroy(&_rwlock);
      _running = false;
      _worker.join();
    }

    bool inSert(const BackupInfo &info)
    {
      pthread_rwlock_wrlock(&_rwlock);
      _table[info.url] = info;
      pthread_rwlock_unlock(&_rwlock);
      ++_changes;
      return true;
    }

    bool inSert(const std::vector<BackupInfo> &info_list)
    {
      pthread_rwlock_wrlock(&_rwlock);
      for(auto &info : info_list)
      {
        _table[info.url] = info;
      }
      pthread_rwlock_unlock(&_rwlock);
      _changes += info_list.size();
      return true;
    }

    bool upDate(const BackupInfo &info)
    {
      pthread_rwlock_wrlock(&_rwlock);
      _table[info.url] = info;
      pthread_rwlock_unlock(&_rwlock);
      ++_changes;
      return true;
    }

    bool getOneByURL(const std::string &url, BackupInfo *info)
    {
      pthread_rwlock_rdlock(&_rwlock);
      auto it = _table.find(url);
      if(it == _table.end())
      {
        pthread_rwlock_unlock(&_rwlock);
        return false;
      }
      *info = it->second;
      pthread_rwlock_unlock(&_rwlock);
      return true;
    }

    bool getAll(std::vector<BackupInfo> *arry)
    {
      pthread_rwlock_rdlock(&_rwlock);
      auto it = _table.begin();
      for(; it != _table.end(); it++)
      {
        arry->push_back(it->second);
      }
      pthread_rwlock_unlock(&_rwlock);
      return true;
    }

    bool InitLoad()
    {
      FileUtil fu(_backup_file);
      if(fu.exits() == false) return true;
      std::string body;
      fu.getContent(&body);
      Json::Value root;
      JsonUtil::deSerialize(body, &root);
      std::vector<BackupInfo> info_list;
      for(int i = 0; i < root.size(); i++)
      {
        BackupInfo info;
        info.pack_flag = root[i]["pack_flag"].asBool();
        info.pack_path = root[i]["pack_path"].asString();
        info.real_path = root[i]["real_path"].asString();
        info.url = root[i]["url"].asString();
        info.file_size = root[i]["file_size"].asUInt();
        info.last_modTime = root[i]["last_modTime"].asLargestInt();
        info_list.push_back(info);
      }
      inSert(info_list);
      return true;
    }
  private:
    bool storage()
    {
      std::vector<BackupInfo> infoList;
      getAll(&infoList);
      Json::Value root;
      for(int i = 0; i < infoList.size(); i++)
      {
        Json::Value item;
        item["pack_flag"] = infoList[i].pack_flag;
        item["pack_path"] = infoList[i].pack_path;
        item["real_path"] = infoList[i].real_path;
        item["url"] = infoList[i].url;
        item["last_modTime"] = infoList[i].last_modTime;
        item["file_size"] = infoList[i].file_size;
        root.append(item);
      }
      std::string body;
      JsonUtil::serialize(root, &body);
      FileUtil fu(_backup_file);
      fu.setContent(body);
      return true;
    }

    void storageThread() 
    {
      auto last_storage_time = std::chrono::steady_clock::now();  // 记录上次持久化时间
      while (_running) 
      {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_storage_time).count();
        if(_changes >= 100 || duration >= 5)
        {
          storage();
          _changes = 0;
          last_storage_time = now;
        }
          
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }

  private:
    std::string _backup_file;
    pthread_rwlock_t _rwlock;
    std::unordered_map<std::string, BackupInfo> _table;

    std::atomic<int> _changes;
    std::thread _worker;
    std::atomic<bool> _running;
  };
}

#endif 
