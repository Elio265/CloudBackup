#ifndef __MY_SERVER__
#define __MY_SERVER__ 

#include "data.hpp"
#include "./lib/httplib.h"

extern wzh::DataManager *_data;
namespace wzh
{
  class server
  {
  public:
    server()
    {
      config *con = config::getInstance();
      _server_port = con->getServerPort();
      _server_ip = con->getServerIp();
      _download_preffix = con->getDownloadPreffix();
    }

    bool RunModule()
    {
        _server.Post("/upload", UpLoad);
        _server.Get("/listshow", ListShow);
        _server.Get("/", ListShow);
        std::string download_url = _download_preffix + "(.*)";
        _server.Get(download_url, DownLoad); // (.*)匹配任意字符任意次
        _server.listen(_server_ip.c_str(), _server_port);
      return true;
    }

  private:
    static void UpLoad(const httplib::Request &req, httplib::Response &res)
    {
      auto ret = req.has_file("file");
      if(ret == false)
      {
        res.status = 400;
        return;
      }

      const auto &file = req.get_file_value("file");
      std::string back_dir = config::getInstance()->getBackDir();
      std::string realpath = back_dir + FileUtil(file.filename).fileName();
      if(FileUtil(back_dir).exits() == false) FileUtil(back_dir).createDirectory();
      FileUtil fu(realpath);
      fu.appContent(file.content);     //追加 
      BackupInfo info;
      info.newBackupInfo(realpath);
      _data->inSert(info);
      return;
    }

    static std::string timetoStr(time_t t)
    {
      std::string tmp = std::ctime(&t);
      return tmp;
    }

    static void ListShow(const httplib::Request &req, httplib::Response &res)
    {
      std::vector<BackupInfo> arry;
      _data->getAll(&arry);
      std::stringstream ss;

      ss << "<html><head><title>Download</title>";
      ss << "<style>";
      ss << "body { font-family: Arial, sans-serif; background-color: #f0f0f0; margin: 0; padding: 0; }";
      ss << "h1 { color: #333; }";
      ss << "table { width: 100%; border-collapse: collapse; margin-top: 20px; }";
      ss << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
      ss << "th { background-color: #f2f2f2; }";
      ss << "h1 { text-align: center; }";
      ss << "a { text-decoration: none; color: #007bff; }";
      ss << "</style>";
      ss << "</head>";
      ss << "<body>";
      ss << "<h1>Download</h1>";
      ss << "<table>";
      ss << "<tr>";
      ss << "<th>File Name</th>";
      ss << "<th>Last Modified</th>";
      ss << "<th>File Size</th>";
      ss << "</tr>";

      for(auto &a : arry)
      {
          ss << "<tr>";
          FileUtil fu(a.real_path);
          std::string filename = fu.fileName();
          ss << "<td><a href='" << a.url << "' class='file-link'>" << filename << "</a></td>";
          ss << "<td align='right' class='date'>" << timetoStr(fu.lastModTime()) << "</td>";
          ss << "<td align='right'>" << fu.fileSize() / 1024 << " KB" << "</td>";
          ss << "</tr>";
      }

      ss << "</table></body></html>";
      res.body = ss.str();
      res.set_header("Content-Type", "text/html");
      res.status = 200;
    }

    static std::string getETag(const BackupInfo &info)
    {
      FileUtil fu(info.real_path);
      std::string etag = fu.fileName();
      etag += "-";
      etag += std::to_string(fu.fileSize());
      etag += "-";
      etag += std::to_string(fu.lastModTime());
      return etag;
    }

    static void DownLoad(const httplib::Request &req, httplib::Response &res)
    {
      BackupInfo info;
      _data->getOneByURL(req.path, &info);
      if(info.pack_flag == true)
      {
        FileUtil fu(info.pack_path);
        fu.unCompress(info.real_path);
        fu.reMove();
        info.pack_flag = false;
        _data->upDate(info);
      }

      bool retrans = false;
      std::string old_etag;
      if(req.has_header("If-Range"))
      {
        old_etag = req.get_header_value("If-Range");
        if(old_etag == getETag(info)) retrans = true;
      }

      FileUtil fu(info.real_path);  
      fu.getContent(&res.body);
      res.set_header("Accept-Ranges", "bytes");
      res.set_header("ETag", getETag(info));
      res.set_header("Content-Type", "application/octet-stream");
      
      if(retrans == false) res.status = 200;
      else res.status = 206;
    }

  private:
    int _server_port;
    std::string _server_ip;
    std::string _download_preffix;
    httplib::Server _server;
  };
}



#endif 
