#include "Util.hpp"
#include "config.hpp"
#include "data.hpp"
#include "HotManager.hpp"
#include "Server.hpp"

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

wzh::DataManager *_data;

int main(int argc, char* argv[])
{
  _data = new wzh::DataManager();
  std::thread thread_hot(hotTest);
  std::thread thread_server(serverTest);
  serverTest();
  return 0;
}
