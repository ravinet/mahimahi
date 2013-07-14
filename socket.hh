#ifndef SOCKET_HH
#define SOCKET_HH

#include <stdint.h>
#include <string>

#include "address.hh"

class Socket
{
private:
  int fd_;

  /* private constants */
  const int listen_backlog_ = 1;

  Address local_addr_, peer_addr_;

public:
  Socket(); /* default constructor */
  ~Socket();

  Socket( const int s_fd, const Address & s_local_addr, const Address & s_peer_addr );

  int fd( void ) { return fd_; } /* XXX */

  void bind( const Address & addr );
  void listen( void );
  void connect( const Address & addr );
  Socket accept( void );

  const Address & local_addr( void ) const { return local_addr_; }
  const Address & peer_addr( void ) const { return peer_addr_; }

  std::string read( void );
};

#endif
