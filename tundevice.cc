/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <functional>

#include "tundevice.hh"
#include "exception.hh"
#include "ezio.hh"
#include "socket.hh"

using namespace std;

TunDevice::TunDevice( const string & name,
                      const string & addr,
                      const string & dstaddr )
    : fd_( open( "/dev/net/tun", O_RDWR ), "open /dev/net/tun" )
{
    auto ioctl_helper = [&]( const int fd,
                             const int request,
                             std::function<void( struct ifreq &ifr,
                                                 struct sockaddr_in &sin )> ifr_adjustment )
    {
        interface_ioctl( fd, request, name, ifr_adjustment );
    };

    ioctl_helper( fd_.num(), TUNSETIFF, [&] ( struct ifreq &ifr,
                                              struct sockaddr_in & )
                  { ifr.ifr_flags = IFF_TUN; } );

    Socket sockfd( SocketType::UDP );

    /* bring interface up */
    ioctl_helper( sockfd.raw_fd(), SIOCSIFFLAGS, [&] ( struct ifreq &ifr,
                                                       struct sockaddr_in &)
                  { ifr.ifr_flags = IFF_UP; } );

    /* assign interface address */
    ioctl_helper( sockfd.raw_fd(), SIOCSIFADDR, [&] ( struct ifreq &ifr,
                                                      struct sockaddr_in &sin )
                  {
                      if ( inet_aton( addr.c_str(), &sin.sin_addr ) == 0 ) {
                          throw Exception( "inet_aton" );
                      }
                      memcpy( &ifr.ifr_addr, &sin, sizeof( struct sockaddr ) );
                  } );
    /* assign destination addresses */
    ioctl_helper( sockfd.raw_fd(), SIOCSIFDSTADDR, [&] ( struct ifreq &ifr,
                                                         struct sockaddr_in &sin )
                  {
                      if ( inet_aton( dstaddr.c_str(), &sin.sin_addr ) == 0 ) {
                          throw Exception( "inet_aton" );
                      }
                      memcpy( &ifr.ifr_dstaddr, &sin, sizeof( struct sockaddr ) );
                  } );
}

void TunDevice::interface_ioctl( const int fd, const int request,
                                 const std::string & name,
                                 std::function<void( struct ifreq &ifr,
                                                     struct sockaddr_in &sin )> ifr_adjustment)
{
    struct ifreq ifr;
    memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

    struct sockaddr_in sin;
    memset( &sin, 0, sizeof( sin ) ); /* does not have default initializer */    
    sin.sin_family = AF_INET;

    ifr_adjustment( ifr, sin );

    if ( ioctl( fd, request, static_cast<void *>( &ifr ) ) < 0 ) {
        throw Exception( "ioctl" );
    }
}
    
