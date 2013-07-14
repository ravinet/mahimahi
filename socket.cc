#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#include "socket.hh"
#include "exception.hh"
#include "address.hh"

using namespace std;

static const size_t read_chunk_size = 1;

Socket::Socket()
  : fd_( socket( AF_INET, SOCK_STREAM, 0 ) ),
    local_addr_(),
    peer_addr_()
{
  if ( fd_ < 0 ) {
    throw Exception( "socket" );
  }
}

Socket::Socket( const int s_fd, const Address & s_local_addr, const Address & s_peer_addr )
  : fd_( s_fd ),
    local_addr_( s_local_addr ),
    peer_addr_( s_peer_addr )
{
}

Socket::~Socket()
{
  if ( close( fd_ ) < 0 ) {
    throw Exception( "close" );
  }
}

void Socket::bind( const Address & addr )
{
  /* make local address to listen on */
  local_addr_ = addr;
 
  /* bind the socket to listen_addr */
  if ( ::bind( fd_,
	       reinterpret_cast<const sockaddr *>( &local_addr_.raw_sockaddr() ),
	       sizeof( local_addr_.raw_sockaddr() ) ) < 0 ) {
    throw Exception( "bind" );
  }
}

void Socket::listen( void )
{
  if ( ::listen( fd_, listen_backlog_ ) < 0 ) {
    throw Exception( "listen" );
  }
}

Socket Socket::accept( void )
{
  /* make new socket address for connection */
  struct sockaddr_in new_connection_addr;
  socklen_t new_connection_addr_size = sizeof( new_connection_addr );

  /* wait for client connection */
  int new_fd = ::accept( fd_,
			 reinterpret_cast<sockaddr *>( &new_connection_addr ),
			 &new_connection_addr_size );

  if ( new_fd < 0 ) {
    throw Exception( "accept" );
  }

  /* verify length is what we expected */
  if ( new_connection_addr_size != sizeof( new_connection_addr ) ) {
    throw Exception( "sockaddr size mismatch" );
  }
  
  return Socket( new_fd, local_addr_, Address( new_connection_addr ) );
}

string Socket::read( void )
{
  char buffer[ read_chunk_size ];

  ssize_t bytes_read = ::read( fd_, &buffer, read_chunk_size );

  if ( bytes_read == 0 ) {
    /* end of file = client has closed their side of connection */
    return string();
  } else if ( bytes_read < 0 ) {
    throw Exception( "read" );
  } else {
    /* successful read */
    return string( buffer, bytes_read );
  }
}

void Socket::connect( const Address & addr )
{
  peer_addr_ = addr;

  if ( ::connect( fd_,
		  reinterpret_cast<const struct sockaddr *>( &peer_addr_.raw_sockaddr() ),
		  sizeof( peer_addr_.raw_sockaddr() ) ) < 0 ) {
    throw Exception( "connect" );
  }
}
