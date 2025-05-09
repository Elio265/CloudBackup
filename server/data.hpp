
#ifndef __MY_DATA__
#define __MY_DATA__

#include <unordered_map>
#include <pthread.h>
#include "Util.hpp"
#include "config.hpp"

namespace wzh
{
  typedef struct BackupInfo
  {
    bool pack_flag;
    // size_t fsize;
    // time_t atime;
    // time_t mtime;
    std::string real_path;
    std::string pack_path; 
    std::string url;

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
      // fsize = fu.fileSize();
      // mtime = fu.lastModTime();
      // atime = fu.lastAccTime();
      real_path = realpath;
      pack_path = packdir + fu.fileName() + packsuffix;
      url = download_preffix + fu.fileName();
      return true;
    }
  }BackupInfo;
  
  class DataManager
  {
  public:
    DataManager()
    {
      _backup_file = config::getInstance()->getBackupFile();
      pthread_rwlock_init(&_rwlock, NULL);
      InitLoad();
    }

    ~DataManager()
    {
      pthread_rwlock_destroy(&_rwlock);
    }

    bool inSert(const BackupInfo &info)
    {
      pthread_rwlock_wrlock(&_rwlock);
      _table[info.url] = info;
      pthread_rwlock_unlock(&_rwlock);
      storage();
      return true;
    }

    bool upDate(const BackupInfo &info)
    {
      pthread_rwlock_wrlock(&_rwlock);
      _table[info.url] = info;
      pthread_rwlock_unlock(&_rwlock);
      storage();
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

    bool getOneByRealPath(const std::string &realpath, BackupInfo *info)
    {
      pthread_rwlock_rdlock(&_rwlock);
      auto it = _table.begin();
      for(; it != _table.end(); it++)
      {
        if(it->second.real_path == realpath)
        {
          *info = it->second;
          pthread_rwlock_unlock(&_rwlock);
          return true;
        }
      }
      pthread_rwlock_unlock(&_rwlock);
      return false;
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

    bool storage()
    {
      std::vector<BackupInfo> arry;
      getAll(&arry);
      Json::Value root;
      for(int i = 0; i < arry.size(); i++)
      {
        Json::Value item;
        item["pack_flag"] = arry[i].pack_flag;
        // item["fsize"] = (Json::Int64)arry[i].fsize;
        // item["atime"] = (Json::Int64)arry[i].atime;
        // item["mtime"] = (Json::Int64)arry[i].mtime;
        item["pack_path"] = arry[i].pack_path;
        item["real_path"] = arry[i].real_path;
        item["url"] = arry[i].url;
        root.append(item);
      }
      std::string body;
      JsonUtil::serialize(root, &body);
      FileUtil fu(_backup_file);
      fu.setContent(body);
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
      for(int i = 0; i < root.size(); i++)
      {
        BackupInfo info;
        info.pack_flag = root[i]["pack_flag"].asBool();
        // info.fsize = root[i]["fsize"].asInt();
        // info.atime = root[i]["atime"].asInt64();
        // info.mtime = root[i]["mtime"].asInt64();
        info.pack_path = root[i]["pack_path"].asString();
        info.real_path = root[i]["real_path"].asString();
        info.url = root[i]["url"].asString();
        inSert(info);
      }
      return true;
    }
  private:
    std::string _backup_file;
    pthread_rwlock_t _rwlock;
    std::unordered_map<std::string, BackupInfo> _table;
  };
}

#endif 
