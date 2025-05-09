#ifndef __MY_CON__
#define __MY_CON__

#include <mutex>
#include "Util.hpp"

#define CONFIG_FILE "./cloud.conf"

namespace wzh
{
  class config
  {
  public:
    static config* getInstance()
    {
      if(_instance == NULL)
      {
        _mutex.lock();
        if(_instance == NULL)
        {
          _instance = new config();
        }
        _mutex.unlock();
      }
      return _instance;
    }

    int getHotTime()
    {
      return _hot_time;
    }

    int getServerPort()
    {
      return _server_port;
    }

    std::string getServerIp()
    {
      return _server_ip;
    }

    std::string getDownloadPreffix()
    {
      return _download_preffix;
    }

    std::string getPackFileSuffix()
    {
      return _pack_suffix;
    }

    std::string getPackDir()
    {
      return _pack_dir;
    }

    std::string getBackDir()
    {
      return _back_dir;
    }

    std::string getBackupFile()
    {
      return _backup_file;
    }

  private:
    config()
    {
      readConfigFile();
    }

    static config* _instance;
    static std::mutex _mutex;

    bool readConfigFile()
    {
      FileUtil fu(CONFIG_FILE);
      std::string body;
      if(fu.getContent(&body) == false)
      {
        std::cout << "readConfigFile failed\n";
        return false;
      }
      Json::Value root;
      if(JsonUtil::deSerialize(body, &root) == false)
      {
        std::cout << "parse config file failed\n";
        return false;
      }
      _hot_time = root["hot_time"].asInt();
      _server_ip = root["server_ip"].asString();
      _server_port = root["server_port"].asInt();
      _pack_dir = root["pack_dir"].asString();
      _back_dir = root["back_dir"].asString();
      _pack_suffix = root["pack_suffix"].asString();
      _download_preffix = root["download_preffix"].asString();
      _backup_file = root["backup_file"].asString();
      return true;
    }
  private:
    int _hot_time;
    int _server_port;
    std::string _server_ip;
    std::string _pack_suffix;
    std::string _pack_dir;
    std::string _back_dir;
    std::string _download_preffix;
    std::string _backup_file;
  };
  config* config::_instance = NULL;
  std::mutex config::_mutex;
}

#endif
