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

Socket::Socket( FileDescriptor && s_fd, const Address & s_local_addr, const Address & s_peer_addr )
  : fd_( s_fd ),
    local_addr_( s_local_addr ),
    peer_addr_( s_peer_addr )
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

pair< Address, string > Socket::recvfrom( void ) const
{
    static const ssize_t RECEIVE_MTU = 2048;

    /* receive source address and payload */
    struct sockaddr_in packet_remote_addr;
    char buf[ RECEIVE_MTU ];

    socklen_t fromlen = sizeof( packet_remote_addr );

    ssize_t recv_len = ::recvfrom( fd_.num(),
                                   buf,
                                   sizeof( buf ),
                                   MSG_TRUNC,
                                   reinterpret_cast< struct sockaddr * >( &packet_remote_addr ),
                                   &fromlen );

    if ( recv_len < 0 ) {
        throw Exception( "recvfrom" );
    } else if ( recv_len > RECEIVE_MTU ) {
        throw Exception( "oversized datagram" );
    }

    return make_pair( Address( packet_remote_addr ),
                      string( buf, recv_len ) );
}

void Socket::sendto( const Address & destination, const std::string & payload ) const
{
    if ( ::sendto( fd_.num(),
                   payload.data(),
                   payload.size(),
                   0,
                   reinterpret_cast<const sockaddr *>( &destination.raw_sockaddr() ),
                   sizeof( destination.raw_sockaddr() ) ) < 0 ) {
        throw Exception( "sendto" );
    }
}
