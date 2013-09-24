/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#include "socket.hh"
#include "exception.hh"
#include "address.hh"
#include "ezio.hh"

using namespace std;

Socket::Socket()
  : fd_( socket( AF_INET, SOCK_STREAM, 0 ), "socket" ),
    local_addr_(),
    peer_addr_()
{
}

void Socket::bind( const Address & addr )
{
  /* make local address to listen on */
  local_addr_ = addr;
 
  /* bind the socket to listen_addr */
  if ( ::bind( fd_.num(),
               reinterpret_cast<const sockaddr *>( &local_addr_.raw_sockaddr() ),
               sizeof( local_addr_.raw_sockaddr() ) ) < 0 ) {
    throw Exception( "bind" );
  }

  /* set local_addr to the address we actually were bound to */
  struct sockaddr_in new_local_addr;
  socklen_t new_local_addr_len = sizeof( new_local_addr );

  if ( ::getsockname( fd_.num(),
                      reinterpret_cast<sockaddr *>( &new_local_addr ),
                      &new_local_addr_len ) < 0 ) {
      throw Exception( "getsockname" );
  }

  local_addr_ = Address( new_local_addr );
}

string Socket::read( void ) const
{
  return fd_.read();
}

void Socket::connect( const Address & addr )
{
  peer_addr_ = addr;

  if ( ::connect( fd_.num(),
		  reinterpret_cast<const struct sockaddr *>( &peer_addr_.raw_sockaddr() ),
		  sizeof( peer_addr_.raw_sockaddr() ) ) < 0 ) {
    throw Exception( "connect" );
  }
}

void Socket::write( const std::string & str ) const
{
  fd_.write( str );
}
