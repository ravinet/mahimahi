/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "tapdevice.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

TapDevice::TapDevice( const std::string & name, const std::string & ipaddr )
    : fd_( open( "/dev/net/tun", O_RDWR ), "open /dev/net/tun" )
{
    struct ifreq ifr;
    memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */

    ifr.ifr_flags = IFF_TAP; /* make tap device */
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

    // create interface
    if ( ioctl( fd().num(), TUNSETIFF, static_cast<void *>( &ifr ) ) < 0 ) {
        throw Exception( "ioctl" );
    }

    /* bring interface up */
    ifr.ifr_flags += IFF_UP;

    FileDescriptor sockfd( socket( AF_INET, SOCK_DGRAM, 0 ), "socket" );

    if ( ioctl( sockfd.num(), SIOCSIFFLAGS, static_cast<void *>( &ifr ) ) < 0 ) {
        throw Exception( "ioctl" );
    }

    /* assign given IP address to interface */
    struct sockaddr_in sin;
    memset( &sin, 0, sizeof( sin ) ); /* does not have default initializer */

    sin.sin_family = AF_INET;

    inet_aton( ipaddr.c_str(), &sin.sin_addr );
    memcpy( &ifr.ifr_addr, &sin, sizeof( struct sockaddr ) );

    if ( ioctl( sockfd.num(), SIOCSIFADDR, static_cast<void *>( &ifr ) ) < 0 ) {
        throw Exception( "ioctl" );
    }
}

