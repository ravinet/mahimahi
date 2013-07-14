#ifndef SOCKET_HH
#define SOCKET_HH

#include <stdint.h>
#include <string>

class Socket
{
private:
  int fd_;

  const int listen_backlog_ = 1;

public:
  Socket();
  ~Socket();

  int fd( void ) { return fd_; }

  void bind( const std::string service ); /* bind socket to INADDR_ANY and specified service */
  void listen( void );
};

#endif
