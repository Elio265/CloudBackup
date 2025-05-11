#include "Util.hpp"
#include "config.hpp"
#include "data.hpp"
#include "HotManager.hpp"
#include "Server.hpp"
#include "user.hpp"
#include <pthread.h>

void serverTest()
{
  wzh::server svr;
  svr.RunModule();
}

void hotTest()
{
  wzh::HotManager hot;
  hot.RunModule();
}

void init()
{
  wzh::config *con = wzh::config::getInstance();
  wzh::FileUtil(con->getBackDir()).createDirectory();
  wzh::FileUtil(con->getPackDir()).createDirectory();
  wzh::UserManager::getInstance();
  _data = new wzh::DataManager();
}

wzh::DataManager *_data;

int main(int argc, char* argv[])
{
  init();
  std::thread thread_hot(hotTest);
  std::thread thread_server(serverTest);
  serverTest();
  return 0;
}
