
#ifndef __MY_HOT__
#define __MY_HOT__ 
#include <unistd.h>
#include <sys/inotify.h>
#include "data.hpp"

extern wzh::DataManager *_data;
namespace wzh
{
  class HotManager
  {
  public:
    HotManager()
    {
      config *con = config::getInstance();
      _back_dir = con->getBackDir();
      _pack_dir = con->getPackDir();
      _pack_suffix = con->getPackFileSuffix();
      _hot_time = con->getHotTime();
    }

    bool RunModule()
    {
      while(1)
      {
        FileUtil fu(_back_dir);
        std::vector<std::string> arry;
        fu.scanDirectory(&arry);
        for(auto &a : arry)
        {
          if(HotJudge(a) == true) continue;

          BackupInfo bi;
          if(_data->getOneByRealPath(a, &bi) == false)
          {
            bi.newBackupInfo(a);
          }
          FileUtil tmp(a);
          tmp.comPress(bi.pack_path);
          tmp.reMove();
          bi.pack_flag = true;
          _data->upDate(bi);
        }
        sleep(_hot_time);
      }
      return true;
    }

  private:
    bool HotJudge(const std::string &filename)
    {
      FileUtil fu(filename);
      time_t last_atime = fu.lastAccTime();
      time_t cur_time = time(NULL);
      if(cur_time - last_atime <= _hot_time) return true;
      return false;
    }

  private:
    int inotify_fd;
    int watfch_id;

    std::atomic<bool> running;

    std::string _back_dir;
    std::string _pack_dir;
    int _hot_time;
    std::string _pack_suffix;
    
  };
}


#endif 
