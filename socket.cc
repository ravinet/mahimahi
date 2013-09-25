/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <utility>
#include <arpa/inet.h>

#include "socket.hh"
#include "exception.hh"
#include "address.hh"
#include "ezio.hh"

using namespace std;

Socket::Socket()
  : fd_( socket( AF_INET, SOCK_DGRAM, 0 ), "socket" ),
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

pair <string, Address> Socket::recv( void ) const
{
  /* receive source address and payload */
  struct sockaddr_in packet_remote_addr;
  char buf[ 2048 ];
  socklen_t fromlen = sizeof( packet_remote_addr );
  ssize_t recv_len = recvfrom( fd_.num(), buf, sizeof( buf ), 0, ( struct sockaddr* )&packet_remote_addr, &fromlen );
  if ( recv_len < 0 ) {
    throw Exception( "recvfrom" );
  }

  /* take hostname and port number for source */
  string source_hostname( inet_ntoa( packet_remote_addr.sin_addr ) );
  string source_port = to_string( packet_remote_addr.sin_port );

  /* create Address object for source and string for payload */
  Address source_addr( source_hostname, source_port );
  string message = string( buf, recv_len );

  std::pair <string, Address> source_info = make_pair ( message, source_addr );
  return ( source_info );
}
