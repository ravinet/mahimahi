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

TapDevice::TapDevice( const string & name,
                      const string & addr,
                      const string & dstaddr )
    : fd_( open( "/dev/net/tun", O_RDWR ), "open /dev/net/tun" )
{
    {
        struct ifreq ifr;
        memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */
        strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

        ifr.ifr_flags = IFF_TUN; /* make tap device */

        // create interface
        if ( ioctl( fd().num(), TUNSETIFF, static_cast<void *>( &ifr ) ) < 0 ) {
            throw Exception( "ioctl" );
        }
    }

    {
        struct ifreq ifr;
        memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */
        strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

        /* bring interface up */
        ifr.ifr_flags += IFF_UP;

        FileDescriptor sockfd( socket( AF_INET, SOCK_DGRAM, 0 ), "socket" );

        if ( ioctl( sockfd.num(), SIOCSIFFLAGS, static_cast<void *>( &ifr ) ) < 0 ) {
            throw Exception( "ioctl" );
        }
    }

    {
        /* assign given IP addresses to interface */
        struct ifreq ifr;
        memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */
        strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

        struct sockaddr_in sin;
        memset( &sin, 0, sizeof( sin ) ); /* does not have default initializer */

        sin.sin_family = AF_INET;

        if ( inet_aton( addr.c_str(), &sin.sin_addr ) == 0 ) {
            throw Exception( "inet_aton" );
        }

        memcpy( &ifr.ifr_addr, &sin, sizeof( struct sockaddr ) );

        FileDescriptor sockfd( socket( AF_INET, SOCK_DGRAM, 0 ), "socket" );

        if ( ioctl( sockfd.num(), SIOCSIFADDR, static_cast<void *>( &ifr ) ) < 0 ) {
            throw Exception( "ioctl" );
        }
    }

    {
        /* assign given destination IP addresses to interface */
        struct ifreq ifr;
        memset( &ifr, 0, sizeof( ifr ) ); /* does not have default initializer */
        strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

        struct sockaddr_in sin;
        memset( &sin, 0, sizeof( sin ) ); /* does not have default initializer */

        sin.sin_family = AF_INET;

        if ( inet_aton( dstaddr.c_str(), &sin.sin_addr ) == 0 ) {
            throw Exception( "inet_aton" );
        }

        memcpy( &ifr.ifr_dstaddr, &sin, sizeof( struct sockaddr ) );

        FileDescriptor sockfd( socket( AF_INET, SOCK_DGRAM, 0 ), "socket" );

        if ( ioctl( sockfd.num(), SIOCSIFDSTADDR, static_cast<void *>( &ifr ) ) < 0 ) {
            throw Exception( "ioctl" );
        }
    }
}

