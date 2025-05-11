#ifndef __MY_UTIL__
#define __MY_UTIL__

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <experimental/filesystem>
#include <jsoncpp/json/json.h>
#include <zstd.h>

namespace wzh
{
  namespace fs = std::experimental::filesystem;
  class FileUtil
  {
  public:
    FileUtil(const std::string filename)
      : _filename(filename) {}

    bool reMove()
    {
      if(exits()) remove(_filename.c_str());
      return true;
    }
  
    int64_t fileSize() //获取文件大小
    {
      struct stat st;
      if(stat(_filename.c_str(), &st) < 0)
      {
        std::cout << "get file size failed\n";
        return -1;
      }
      return st.st_size;
    }

    time_t lastModTime() //获取文件最后一次修改时间
    {
      struct stat st;
      if(stat(_filename.c_str(), &st) < 0)
      {
        std::cout << "get file lastModTime failed\n";
        return -1;
      }
      return st.st_mtime;
    }

    time_t lastAccTime() //获取最后一次访问时间
    {
      struct stat st;
      if(stat(_filename.c_str(), &st) < 0)
      {
        std::cout << "get file lastAccTime failed\n";
        return -1;
      }
      return st.st_atime;
    }

    std::string fileName() //获取文件名称
    {
      size_t pos = _filename.find_last_of("/");
      if(pos == std::string::npos)
      {
        return _filename;
      }
      return _filename.substr(pos + 1);
    }

    bool setContent(const std::string &body) //向文件写入数据
    {
      std::ofstream ofs;
      ofs.open(_filename, std::ios::binary);
      if(ofs.is_open() == false)
      {
        std::cout << "write open failed\n";
        return false;
      }
      ofs.write(&body[0], body.size());
      if(ofs.good() == false)
      {
        std::cout << "write file content failed\n";
        ofs.close();
        return false;
      }
      ofs.close();
      return true;
    }

    bool getContent(std::string *body) //向文件读取数据
    {
      size_t fsize = fileSize();
      return getPosLen(body, 0, fsize);
    }

    bool getPosLen(std::string *body, size_t pos, size_t len) //获取文件指定位置，指定长度的数据
    {
      std::ifstream ifs;
      ifs.open(_filename, std::ios::binary);
      if(ifs.is_open() == false)
      {
        std::cout << "read open file failed\n";
        return false;
      }

      size_t fsize = fileSize();
      if(pos + len > fsize)
      {
        std::cout << "get file len is error\n";
        return false;
      }
      ifs.seekg(pos, std::ios::beg);
      body->resize(len);
      ifs.read(&(*body)[0], len);
      if(ifs.good() == false)
      {
        std::cout << "get file content failed\n";
        ifs.close();
        return false;
      }
      ifs.close();
      return true;
    }

    bool exits() // 判断（文件/目录）是否存在
    {
      return fs::exists(_filename);     
    }

    bool scanDirectory(std::vector<std::string> *arry) //浏览目录中的所有文件
    {
      for(auto& p : fs::directory_iterator(_filename))
      {
        if(fs::is_directory(p) == true)
          continue;
        arry->push_back(fs::path(p).relative_path().string());
      } 
      return true;
    }

    bool createDirectory() //创建目录
    {
      if(exits()) return true;
      return fs::create_directories(_filename);
    }

    bool comPress(const std::string &packname) //压缩文件
    {
      std::string body;
      if(getContent(&body) == false)
      {
        std::cout << "compress get file content failed\n";
        return false;
      }
      //计算最大压缩大小
      size_t maxCompressedSize = ZSTD_compressBound(body.size());
      std::string compressed(maxCompressedSize, '\0');
      //压缩
      size_t compressedSize = ZSTD_compress(const_cast<char*>(compressed.data()), maxCompressedSize, body.data(), body.size(), 3);
      if(ZSTD_isError(compressedSize))
      {
        std::cout << "Zstd compression failed\n";
        return false;
      }
      compressed.resize(compressedSize); // 裁剪字符串，只保留有效数据
      FileUtil fu(packname);
      if(fu.setContent(compressed) == false)
      {
        std::cout << "comPress write packed data failed\n";
        return false;
      }
      return true;
    }

    bool unCompress(const std::string &filename) //解压文件
    {
      std::string body;
      if(getContent(&body) == false)
      {
        std::cout << "unCompress get file content failed\n";
        return false;
      }

      unsigned long long decompressedSize = ZSTD_getFrameContentSize(body.data(), body.size());
      if(decompressedSize == ZSTD_CONTENTSIZE_ERROR || decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
          std::cout << "Zstd couldn't determine original size\n";
          return false;
      }
      // size_t decompressedSize = body.size() * 10;
      std::vector<char> decompressed(decompressedSize);
      // 解压数据
      size_t actualDecompressedSize = ZSTD_decompress(decompressed.data(), decompressedSize, body.data(), body.size());
      
      if(ZSTD_isError(actualDecompressedSize))
      {
        std::cout << "Zstd decompression failed: " << ZSTD_getErrorName(actualDecompressedSize) << "\n";
        return false;
      }

      FileUtil fu(filename);
      if(fu.setContent(std::string(decompressed.data(), actualDecompressedSize)) == false)
      {
        std::cout << "unCompress set file ccontent failed\n";
        return false;
      }
      return true;
    }

  private:
    std::string _filename;
  };

  class JsonUtil
  {
  public:
    static bool serialize(const Json::Value &root, std::string *str)
    {
      Json::StreamWriterBuilder swb;
      std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
      std::stringstream ss;
      if(sw->write(root, &ss) != 0)
      {
        std::cout << "serialize write failed\n";
        return false;
      }
      *str = ss.str();
      return true;
    }

    static bool deSerialize(const std::string &str, Json::Value *root)
    {
      Json::CharReaderBuilder crb;
      std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
      std::string err;
      bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), root, &err);
      if(ret == false)
      {
        std::cout << "deSerialize parse error: " << err << std::endl;
        return false;
      }
      return true;
    }
  };
}

#endif
