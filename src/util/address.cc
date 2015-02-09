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
        throw runtime_error( "Address(): sockaddr is not of family AF_INET" );
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

bool Address::operator<( const Address & other ) const
{
    return (memcmp( &addr_, &other.addr_, sizeof( addr_ ) ) < 0 );
}

Address::Address( const string & ip, const uint16_t port )
    : addr_()
{
    addr_.sin_family = AF_INET;

    if ( 1 != inet_pton( AF_INET, ip.c_str(), &addr_.sin_addr ) ) {
        throw unix_error( "inet_pton (" + ip + ")" );
    }

    addr_.sin_port = htons( port );
}

Address Address::cgnat( const uint8_t last_octet )
{
    return Address( "100.64.0." + to_string( last_octet ), 0 );
}

/* error category for getaddrinfo */
class gai_error_category : public error_category
{
public:
    const char * name( void ) const noexcept override { return "getaddrinfo"; }
    string message( const int gai_ret ) const noexcept override
    {
        return gai_strerror( gai_ret );
    }
};

Address::Address( const string & hostname, const string & service )
  : addr_()
{
  /* give hints to resolver */
  addrinfo hints;
  zero( hints );
  hints.ai_family = AF_INET;

  /* prepare for the answer */
  addrinfo *res;

  /* look up the name or names */
  int gai_ret = getaddrinfo( hostname.c_str(), service.c_str(), &hints, &res );
  if ( gai_ret != 0 ) {
      throw tagged_error( gai_error_category(), "getaddrinfo", gai_ret );
  }

  /* if success, should always have at least one entry */
  assert( res );
  
  /* should match our request */
  assert( res->ai_family == AF_INET );
  assert( res->ai_addrlen == sizeof( addr_ ) );

  /* assign to our private member variable */
  addr_ = *reinterpret_cast<sockaddr_in *>( res->ai_addr );

  freeaddrinfo( res );
}

string Address::str( const string port_separator ) const
{
  return ip() + port_separator + to_string( port() );
}

uint16_t Address::port( void ) const
{
  return ntohs( addr_.sin_port );
}

string Address::ip( void ) const
{
    char addrstr[ INET_ADDRSTRLEN ] = {};
    if ( nullptr == inet_ntop( AF_INET, &addr_.sin_addr, addrstr, INET_ADDRSTRLEN ) ) {
        throw unix_error( "inet_ntop" );
    }

    return addrstr;
}
