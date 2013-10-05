/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>

#include "address.hh"
#include "exception.hh"
#include "socket.hh"

using namespace std;

Address::Address( const struct sockaddr_in &s_addr )
  : addr_( s_addr )
{
}

Address::Address()
    : addr_()
{
    memset( &addr_, 0, sizeof( addr_ ) );
}

Address::Address( const std::string & ip, const uint16_t port )
    : addr_()
{
    addr_.sin_family = AF_INET;

    if ( 1 != inet_pton( AF_INET, ip.c_str(), &addr_.sin_addr ) ) {
        throw Exception( "inet_pton" );
    }

    addr_.sin_port = htons( port );
}


Address::Address( const std::string & hostname, const std::string & service, const SocketType & socket_type )
  : addr_()
{
  /* give hints to resolver */
  struct addrinfo hints;
  memset( &hints, 0, sizeof( hints ) );
  hints.ai_family = AF_INET;
  hints.ai_socktype = socket_type;

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
  assert( res->ai_socktype == socket_type );
  assert( res->ai_addrlen == sizeof( addr_ ) );

  /* assign to our private member variable */
  addr_ = *reinterpret_cast<sockaddr_in *>( res->ai_addr );

  freeaddrinfo( res );
}

string Address::str( void ) const
{
  return ip() + ":" + to_string( port() );
}

uint16_t Address::port( void ) const
{
  return ntohs( addr_.sin_port );
}

string Address::ip( void ) const
{
    char addrstr[ INET_ADDRSTRLEN ] = {};
    if ( nullptr == inet_ntop( AF_INET, &addr_.sin_addr, addrstr, INET_ADDRSTRLEN ) ) {
        throw Exception( "inet_ntop" );
    }

    return addrstr;
}
