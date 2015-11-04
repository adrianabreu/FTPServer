#if !defined FTPServer_H
#define FTPServer_H

#include <list>


#include "ClientConnection.h"

class FTPServer {
public:
  FTPServer(int port = 2121);
  void run();
  void stop();

private:
  int port;
  int msock; //ER SOCKET
  std::list<ClientConnection*> connection_list;
};

#endif
