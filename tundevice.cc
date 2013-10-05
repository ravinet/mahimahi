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
#include "util.hh"

using namespace std;

TunDevice::TunDevice( const string & name,
                      const string & addr,
                      const string & dstaddr )
    : fd_( open( "/dev/net/tun", O_RDWR ), "open /dev/net/tun" )
{
    interface_ioctl( fd_.num(), TUNSETIFF, name,
                     [] ( struct ifreq &ifr ) { ifr.ifr_flags = IFF_TUN; } );

    Socket sockfd( SocketType::UDP );

    /* bring interface up */
    interface_ioctl( sockfd.raw_fd(), SIOCSIFFLAGS, name,
                     [] ( struct ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

    /* assign interface address */
    interface_ioctl( sockfd.raw_fd(), SIOCSIFADDR, name,
                     [&] ( struct ifreq &ifr )
                     { ifr.ifr_addr = Address( addr, 0 ).raw_sockaddr(); } );

    /* assign destination addresses */
    interface_ioctl( sockfd.raw_fd(), SIOCSIFDSTADDR, name,
                     [&] ( struct ifreq &ifr )
                     { ifr.ifr_dstaddr = Address( dstaddr, 0 ).raw_sockaddr(); } );
}

void TunDevice::interface_ioctl( const int fd, const int request,
                                 const std::string & name,
                                 std::function<void( struct ifreq &ifr )> ifr_adjustment)
{
    struct ifreq ifr;
    zero( ifr );
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

    ifr_adjustment( ifr );

    if ( ioctl( fd, request, static_cast<void *>( &ifr ) ) < 0 ) {
        throw Exception( "ioctl" );
    }
}
    
