/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <functional>

#include "netdevice.hh"
#include "exception.hh"
#include "ezio.hh"
#include "socket.hh"
#include "util.hh"
#include "system_runner.hh"

#include "config.h"

using namespace std;

TunDevice::TunDevice( const string & name,
                      const string & addr,
                      const string & dstaddr )
    : fd_( open( "/dev/net/tun", O_RDWR ), "open /dev/net/tun" )
{
    interface_ioctl( fd_, TUNSETIFF, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_TUN; } );

    Socket sock( UDP );

    /* bring interface up */
    interface_ioctl( sock.fd(), SIOCSIFFLAGS, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

    /* assign interface address */
    interface_ioctl( sock.fd(), SIOCSIFADDR, name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = Address( addr, 0 ).raw_sockaddr(); } );

    /* assign destination addresses */
    interface_ioctl( sock.fd(), SIOCSIFDSTADDR, name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_dstaddr = Address( dstaddr, 0 ).raw_sockaddr(); } );
}

void interface_ioctl( FileDescriptor & fd, const int request,
                      const std::string & name,
                      std::function<void( ifreq &ifr )> ifr_adjustment)
{
    ifreq ifr;
    zero( ifr );
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

    ifr_adjustment( ifr );

    if ( ioctl( fd.num(), request, static_cast<void *>( &ifr ) ) < 0 ) {
        throw Exception( "ioctl " + name );
    }
}

void assign_address( const string & device_name, const Address & addr )
{
    Socket ioctl_socket( UDP );

    /* assign addresses */
    interface_ioctl( ioctl_socket.fd(), SIOCSIFADDR, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = addr.raw_sockaddr(); } );

    /* bring interface up */
    interface_ioctl( ioctl_socket.fd(), SIOCSIFFLAGS, device_name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
}

void name_check( const string & str )
{
    if ( str.find( "veth-" ) != 0 ) {
        throw Exception( str, "name of veth device must start with \"veth-\"" );
    }
}

VirtualEthernetPair::VirtualEthernetPair( const string & outside_name, const string & inside_name )
    : name( outside_name )
{
    /* make pair of veth devices */
    name_check( outside_name );
    name_check( inside_name );

    run( { IP, "link", "add", outside_name, "type", "veth", "peer", "name", inside_name } );

    /* turn off arp */
    run( { IP, "link", "set", "dev", outside_name, "arp", "off" } );
    run( { IP, "link", "set", "dev", inside_name, "arp", "off" } );
}

VirtualEthernetPair::~VirtualEthernetPair()
{
    run( { IP, "link", "del", name } );
    /* deleting one is sufficient to delete both */
}
