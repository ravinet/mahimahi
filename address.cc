/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <arpa/inet.h>

#include "address.hh"
#include "exception.hh"
#include "socket.hh"
#include "util.hh"

using namespace std;

Address::Address( const sockaddr_in & s_addr )
    : addr_( s_addr )
{
}

Address::Address( const sockaddr & s_addr )
    : addr_()
{
    if ( s_addr.sa_family != AF_INET ) {
        throw Exception( "Address()", "sockaddr is not of family AF_INET" );
    }

    addr_ = *reinterpret_cast<const sockaddr_in *>( &s_addr );
}

Address::Address()
    : addr_()
{
    zero( addr_ );
    addr_.sin_family = AF_INET;
}

bool Address::operator==( const Address & other ) const
{
    return 0 == memcmp( &addr_, &other.addr_, sizeof( addr_ ) );
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

Address Address::cgnat( const uint8_t last_octet )
{
    return Address( "100.64.0." + to_string( last_octet ), 0 );
}

Address::Address( const std::string & hostname, const std::string & service, const SocketType & socket_type )
  : addr_()
{
  /* give hints to resolver */
  addrinfo hints;
  zero( hints );
  hints.ai_family = AF_INET;
  hints.ai_socktype = socket_type;

  /* prepare for the answer */
  addrinfo *res;

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
