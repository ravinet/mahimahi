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
  if ( fd_.num() < 0 ) {
    cout << "fails" << endl;
    throw Exception( "socket" );
  }
}

Socket::~Socket()
{
  cout << "deconstructor called" << endl;
  fd_.~FileDescriptor();
}

int Socket::bind( const Address & addr )
{
  /* allow quicker reuse of local address */
  int true_value = true;
  if ( setsockopt( fd_.num(), SOL_SOCKET, SO_REUSEADDR, &true_value, sizeof( true_value ) ) < 0 ) {
    throw Exception( "setsockopt" );
  }

  /* make local address to listen on */
  local_addr_ = addr;
 
  /* bind the socket to listen_addr */
  if ( ::bind( fd_.num(),
	       reinterpret_cast<const sockaddr *>( &local_addr_.raw_sockaddr() ),
	       sizeof( local_addr_.raw_sockaddr() ) ) < 0 ) {
    throw Exception( "bind" );
  }
  /* return port */
  socklen_t length = sizeof( local_addr_.raw_sockaddr() );
  int sock_name;
  sock_name = getsockname( fd_.num(), ( struct sockaddr* )&local_addr_.raw_sockaddr(), &length );
  return( sock_name );
}

void Socket::listen( void )
{
  if ( ::listen( fd_.num(), listen_backlog_ ) < 0 ) {
    throw Exception( "listen" );
  }
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
