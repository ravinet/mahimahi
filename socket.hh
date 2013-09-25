/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SOCKET_HH
#define SOCKET_HH

#include <stdint.h>
#include <string>
#include <utility>

#include "address.hh"
#include "file_descriptor.hh"

class Socket
{
private:
  FileDescriptor fd_;

  Address local_addr_, peer_addr_;

public:
  Socket(); /* default constructor */

  void bind( const Address & addr );
  void connect( const Address & addr );
  Socket accept( void );

  const Address & local_addr( void ) const { return local_addr_; }
  const Address & peer_addr( void ) const { return peer_addr_; }

  std::string read( void ) const;
  void write( const std::string & str ) const;

  int raw_fd( void ) const { return fd_.num(); }

  std::pair <Address, std::string> recv( void ) const;
};

#endif
