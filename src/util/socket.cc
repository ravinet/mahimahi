/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <cassert>

#include "socket.hh"
#include "exception.hh"
#include "address.hh"
#include "ezio.hh"

using namespace std;

Socket::Socket( const SocketType & socket_type )
    : FileDescriptor( SystemCall( "socket", socket( AF_INET, socket_type, 0 ) ) ),
      local_addr_(),
      peer_addr_()
{
}

Socket::Socket( FileDescriptor && s_fd, const Address & s_local_addr, const Address & s_peer_addr )
  : FileDescriptor( move( s_fd ) ),
    local_addr_( s_local_addr ),
    peer_addr_( s_peer_addr )
{
}

void Socket::bind( const Address & addr )
{
    /* make local address to listen on */
    local_addr_ = addr;
 
    /* bind the socket to listen_addr */
    SystemCall( "bind", ::bind( num(),
                                &local_addr_.raw_sockaddr(),
                                sizeof( local_addr_.raw_sockaddr() ) ) );

    /* set local_addr to the address we actually were bound to */
    sockaddr_in new_local_addr;
    socklen_t new_local_addr_len = sizeof( new_local_addr );

    SystemCall( "getsockname", ::getsockname( num(),
                                              reinterpret_cast<sockaddr *>( &new_local_addr ),
                                              &new_local_addr_len ) );

    local_addr_ = Address( new_local_addr );
}

static const int listen_backlog_ = 16;

void Socket::listen( void )
{
    SystemCall( "listen", ::listen( num(), listen_backlog_ ) );
}

Socket Socket::accept( void )
{
  /* make new socket address for connection */
  sockaddr_in new_connection_addr;
  socklen_t new_connection_addr_size = sizeof( new_connection_addr );

  /* wait for client connection */
  FileDescriptor new_fd( SystemCall( "accept",
                                     ::accept( num(),
                                               reinterpret_cast<sockaddr *>( &new_connection_addr ),
                                               &new_connection_addr_size ) ) );

  // verify length is what we expected 
  if ( new_connection_addr_size != sizeof( new_connection_addr ) ) {
    throw runtime_error( "sockaddr size mismatch" );
  }

  register_read();
  
  return Socket( move( new_fd ), local_addr_, Address( new_connection_addr ) );
}

void Socket::connect( const Address & addr )
{
    peer_addr_ = addr;

    SystemCall( "connect", ::connect( num(),
                                      &peer_addr_.raw_sockaddr(),
                                      sizeof( peer_addr_.raw_sockaddr() ) ) );
}

pair< Address, string > Socket::recvfrom( void )
{
    static const ssize_t RECEIVE_MTU = 2048;

    /* receive source address and payload */
    sockaddr_in packet_remote_addr;
    char buf[ RECEIVE_MTU ];

    socklen_t fromlen = sizeof( packet_remote_addr );

    ssize_t recv_len = ::recvfrom( num(),
                                   buf,
                                   sizeof( buf ),
                                   MSG_TRUNC,
                                   reinterpret_cast< sockaddr * >( &packet_remote_addr ),
                                   &fromlen );

    if ( recv_len < 0 ) {
        throw unix_error( "recvfrom" );
    } else if ( recv_len > RECEIVE_MTU ) {
        throw unix_error( "recvfrom (oversized datagram)" );
    }

    register_read();

    return make_pair( Address( packet_remote_addr ),
                      string( buf, recv_len ) );
}

void Socket::sendto( const Address & destination, const string & payload )
{
    SystemCall( "sendto", ::sendto( num(),
                                    payload.data(),
                                    payload.size(),
                                    0,
                                    &destination.raw_sockaddr(),
                                    sizeof( destination.raw_sockaddr() ) ) );

    register_write();
}

void Socket::getsockopt( const int level, const int optname,
                        void *optval, socklen_t *optlen ) const
{
    SystemCall( "getsockopt", ::getsockopt( const_cast<Socket &>( *this ).num(), level, optname, optval, optlen ) );
}

Address Socket::original_dest( void ) const
{
    sockaddr_in dstaddr;
    socklen_t destlen = sizeof( dstaddr );
    getsockopt( SOL_IP, SO_ORIGINAL_DST, &dstaddr, &destlen );
    assert( destlen == sizeof( dstaddr ) );
    return dstaddr;
}

Socket::Socket( Socket && other )
    : FileDescriptor( move( other ) ),
      local_addr_( other.local_addr_),
      peer_addr_( other.peer_addr_ ) {}
