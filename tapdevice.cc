/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <functional>

#include "tapdevice.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

TapDevice::TapDevice( const string & name,
                      const string & addr,
                      const string & dstaddr )
    : fd_( open( "/dev/net/tun", O_RDWR ), "open /dev/net/tun" )
{
    auto ioctl_helper = [&]( FileDescriptor & ioctl_fd,
                             const int ioctl_request,
                             std::function<void( struct ifreq &ifr,
                                                 struct sockaddr_in &sin )> ifr_adjustment )
    {
        struct ifreq ifr;
        memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */
        strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

        struct sockaddr_in sin;
        memset( &sin, 0, sizeof( sin ) ); /* does not have default initializer */    
        sin.sin_family = AF_INET;

        ifr_adjustment( ifr, sin );

        if ( ioctl( ioctl_fd.num(), ioctl_request, static_cast<void *>( &ifr ) ) < 0 ) {
            throw Exception( "ioctl" );
        }
    };

    ioctl_helper( fd_, TUNSETIFF, [&] ( struct ifreq &ifr,
                                        struct sockaddr_in & )
                  { ifr.ifr_flags = IFF_TUN; } );

    FileDescriptor sockfd( socket( AF_INET, SOCK_DGRAM, 0 ), "socket" );

    /* bring interface up */
    ioctl_helper( sockfd, SIOCSIFFLAGS, [&] ( struct ifreq &ifr,
                                              struct sockaddr_in &)
                  { ifr.ifr_flags = IFF_UP; } );

    /* assign interface address */
    ioctl_helper( sockfd, SIOCSIFADDR, [&] ( struct ifreq &ifr,
                                             struct sockaddr_in &sin )
                  {
                      if ( inet_aton( addr.c_str(), &sin.sin_addr ) == 0 ) {
                          throw Exception( "inet_aton" );
                      }
                      memcpy( &ifr.ifr_addr, &sin, sizeof( struct sockaddr ) );
                  } );
    /* assign destination addresses */
    ioctl_helper( sockfd, SIOCSIFDSTADDR, [&] ( struct ifreq &ifr,
                                                struct sockaddr_in &sin )
                  {
                      if ( inet_aton( dstaddr.c_str(), &sin.sin_addr ) == 0 ) {
                          throw Exception( "inet_aton" );
                      }
                      memcpy( &ifr.ifr_dstaddr, &sin, sizeof( struct sockaddr ) );
                  } );
}

