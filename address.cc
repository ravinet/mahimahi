#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>

#include "address.hh"
#include "exception.hh"

Address::Address( const struct sockaddr_in *s_addr )
  : addr_( *s_addr )
{
}

Address::Address( const std::string hostname, const std::string service )
  : addr_()
{
  /* give hints to resolver */
  struct addrinfo hints;
  memset( &hints, 0, sizeof( struct addrinfo ) );
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  /* prepare for the answer */
  struct addrinfo *res;

  /* look up the name or names */
  int gai_ret = getaddrinfo( hostname.c_str(), service.c_str(), &hints, &res );
  if ( gai_ret != 0 ) {
    throw Exception( "getaddrinfo", gai_strerror( gai_ret ) );
  }

  /* if success, should always have at least one entry */
  assert( res );
  
  /* should match our request */
  assert( res->ai_family == AF_INET );
  assert( res->ai_socktype == SOCK_STREAM );
  assert( res->ai_addrlen == sizeof( addr_ ) );

  /* assign to our private member variable */
  addr_ = *reinterpret_cast<sockaddr_in *>( res->ai_addr );
}
