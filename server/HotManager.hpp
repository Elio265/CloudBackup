
#ifndef __MY_HOT__
#define __MY_HOT__ 
#include <unistd.h>
#include <sys/inotify.h>
#include "data.hpp"
#include <thread>
#include <atomic>

extern wzh::DataManager *_data;
namespace wzh
{
  class HotManager
  {
  public:
    HotManager() :inotify_fd(-1), watch_fd(-1), running(false)
    {
      config *con = config::getInstance();
      _back_dir = con->getBackDir();
      _pack_dir = con->getPackDir();
      _pack_suffix = con->getPackFileSuffix();
      _hot_time = con->getHotTime();
      _downloadPreffix = con->getDownloadPreffix();
      _backup_file = con->getBackupFile();

      inotify_fd = inotify_init(); //BLOCK
      if(inotify_fd < 0)
      {
        perror("inotify init failed");
        exit(-1);
      }
      // 添加监控事件
      watch_fd = inotify_add_watch(inotify_fd, _back_dir.c_str(), 
                                  IN_ACCESS | IN_MODIFY | IN_CREATE | IN_DELETE);
      if(watch_fd < 0)
      {
        close(inotify_fd);
        perror("inotify add watch failed");
        exit(-1);
      }
    }

    ~HotManager()
    {
      running = false;
      if(watch_fd != -1) inotify_rm_watch(inotify_fd, watch_fd);
      if(inotify_fd != -1) close(inotify_fd);
    }

    bool RunModule()
    {
      running = true;

      std::thread listener([this](){EventListener();});
      listener.detach();

      while(running)
      {
        ProcessHotFiles();
        std::this_thread::sleep_for(std::chrono::seconds(_hot_time));
      }
      return true;
    }

  private:
    void EventListener()
    {
      constexpr size_t BUF_LEN = 4096;
      char buffer[BUF_LEN];

      while (running) 
      {
        ssize_t len = read(inotify_fd, buffer, BUF_LEN);
        if (len < 0) 
        {
          perror("read");
          break;
        }

        // 处理事件
        std::lock_guard<std::mutex> lock(cache_mutex);
        for (char *ptr = buffer; ptr < buffer + len;) 
        {
          struct inotify_event *event = reinterpret_cast<struct inotify_event*>(ptr);
          if (event->len > 0) 
          {
            std::string filepath = _back_dir + event->name;

            if (event->mask & (IN_ACCESS | IN_MODIFY | IN_CREATE)) // 文件新建、被访问或修改，更新时间
            {
              last_access_map[filepath] = time(nullptr);
            } 
            else if (event->mask & IN_DELETE) // 文件删除，移除记录
            {
              last_access_map.erase(filepath);
            }
          }
          ptr += sizeof(struct inotify_event) + event->len;
        }
      }
    }

    void ProcessHotFiles()
    {
      // 创建本地副本，避免长时间持有锁
      std::unordered_map<std::string, time_t> local_copy;
      {
        std::lock_guard<std::mutex> lock(cache_mutex);
        local_copy = last_access_map;
      }

      // 检查并压缩冷文件
      time_t now = time(nullptr);
      for (const auto &[filepath, last_access] : local_copy) 
      {
        if (now - last_access > _hot_time) 
        {
          CompressFile(filepath);
          {
            std::lock_guard<std::mutex> lock(cache_mutex);
            last_access_map.erase(filepath);
          }
        }
      }
    }

    void CompressFile(const std::string &filepath)
    {
      BackupInfo bi;
      std::string fileName = filepath.substr(filepath.find_last_of("/") + 1);
      std::string url = _downloadPreffix + fileName;
      if (!_data->getOneByURL(url, &bi)) 
      {
        bi.newBackupInfo(filepath);  // 创建备份信息
      }

      FileUtil fu(filepath);
      if (fu.comPress(bi.pack_path)) 
      { 
        fu.reMove();                    // 删除原文件
        bi.pack_flag = true;             // 更新状态
        _data->upDate(bi);               // 更新备份信息
      }
    }

  private:
    int inotify_fd;
    int watch_fd;

    std::mutex cache_mutex;
    std::unordered_map<std::string, time_t> last_access_map;

    std::string _back_dir;
    std::string _pack_dir;
    int _hot_time;
    std::string _pack_suffix;
    std::string _downloadPreffix;
    std::string _backup_file;

    std::atomic<bool> running;  // 控制线程运行
  };
}


#endif 
