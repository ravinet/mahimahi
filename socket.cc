#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#include "socket.hh"
#include "exception.hh"
#include "address.hh"

using namespace std;

Socket::Socket()
  : fd_( socket( AF_INET, SOCK_STREAM, 0 ) )
{
  if ( fd_ < 0 ) {
    throw Exception( "socket" );
  }
}

Socket::~Socket()
{
  if ( close( fd_ ) < 0 ) {
    throw Exception( "close" );
  }
}

void Socket::bind( const string service )
{
  /* make local address to listen on */
  Address listen_addr( "0", service );
 
  /* bind the socket to listen_addr */
  if ( ::bind( fd_,
	       reinterpret_cast<const sockaddr *>( &listen_addr.raw_sockaddr() ),
	       sizeof( listen_addr ) ) < 0 ) {
    throw Exception( "bind" );
  }
}

void Socket::listen( void )
{
  if ( ::listen( fd_, listen_backlog_ ) < 0 ) {
    throw Exception( "listen" );
  }
}
