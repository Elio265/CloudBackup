#ifndef __MY_SERVER__
#define __MY_SERVER__ 

#include "data.hpp"
#include "./lib/httplib.h"
#include "user.hpp"

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
      // 注册接口
      _server.Post("/register", [](auto& req, auto& res)
      {
          // 1. 解析表单参数
          auto u = req.get_param_value("username");
          auto p = req.get_param_value("password");
          
          // 2. 业务处理
          bool ok = UserManager::getInstance().registerUser(u, p);
          
          // 3. 设置响应
          res.status = ok ? 200 : 400;
          res.set_header("Content-Type", "application/json");
          if (ok) 
          {
            res.set_content(R"({"result":"success","message":"注册成功"})",
                            "application/json");
          } 
          else 
          {
              res.set_content(R"({"result":"user_exists","message":"用户已存在"})",
                              "application/json");
          }
      });

      // 登录接口
      _server.Post("/login", [](auto& req, auto& res)
      {
          auto u = req.get_param_value("username");
          auto p = req.get_param_value("password");
          bool ok = UserManager::getInstance().loginUser(u, p);

          res.status = ok ? 200 : 401;
          res.set_header("Content-Type", "application/json");
          if (ok) 
          {
            res.set_content(R"({"result":"success","message":"登录成功"})",
                            "application/json");
          } 
          else 
          {
              res.set_content(R"({"result":"invalid_credentials","message":"用户名或密码错误"})",
                              "application/json");
          }
      });
      _server.Post("/upload", UpLoad);
      _server.Get("/listshow", ListShow);
      _server.Get("/", ListShow);
      _server.Get("/listjson", ListJson); // 新增json接口
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
      fu.setContent(file.content);    
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

    static void ListJson(const httplib::Request &req, httplib::Response &res)
    {
        std::vector<BackupInfo> arry;
        _data->getAll(&arry);
        std::stringstream ss;
        ss << "[";

        for(size_t i=0; i<arry.size(); ++i) {
            auto &a = arry[i];
            FileUtil fu(a.real_path);

            ss << "{";
            ss << "\"filename\":\"" << fu.fileName() << "\",";
            ss << "\"url\":\"" << a.url << "\",";
            ss << "\"last_modTime\":" << a.last_modTime << ",";
            ss << "\"file_size\":" << a.file_size;
            ss << "}";
            if(i+1 != arry.size()) ss << ",";
        }
        ss << "]";
        res.body = ss.str();
        res.set_header("Content-Type", "application/json");
        res.status = 200;
    }

    // 工具函数：HTML转义
    static std::string html_escape(const std::string &str) {
      std::string out;
      for (char c : str) {
          switch (c) {
              case '&': out += "&amp;"; break;
              case '<': out += "&lt;"; break;
              case '>': out += "&gt;"; break;
              case '"': out += "&quot;"; break;
              case '\'': out += "&#39;"; break;
              default: out += c;
          }
      }
      return out;
    }

    static void ListShow(const httplib::Request &req, httplib::Response &res)
    {
      std::vector<BackupInfo> arry;
      _data->getAll(&arry);
      std::stringstream ss;
  
      ss << "<html><head><meta charset=\"UTF-8\"><title>Download</title>";
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
  
      for (auto &a : arry)
      {
          ss << "<tr>";
          FileUtil fu(a.real_path);
          std::string filename = html_escape(fu.fileName());
          ss << "<td><a href='" << a.url << "' class='file-link'>" << filename << "</a></td>";
          ss << "<td align='right' class='date'>" << timetoStr(a.last_modTime) << "</td>";
          ss << "<td align='right'>" << a.file_size / 1024 << " KB" << "</td>";
          ss << "</tr>";
      }
  
      ss << "</table></body></html>";
      res.body = ss.str();
      res.set_header("Content-Type", "text/html; charset=UTF-8");
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
