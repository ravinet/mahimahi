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
                      const Address & addr,
                      const Address & peer )
    : fd_( SystemCall( "open /dev/net/tun", open( "/dev/net/tun", O_RDWR ) ) )
{
    interface_ioctl( fd_, TUNSETIFF, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_TUN; } );

    assign_address( name, addr, peer );
}

void interface_ioctl( FileDescriptor & fd, const int request,
                      const string & name,
                      function<void( ifreq &ifr )> ifr_adjustment)
{
    ifreq ifr;
    zero( ifr );
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

    ifr_adjustment( ifr );

    SystemCall( "ioctl " + name, ioctl( fd.num(), request, static_cast<void *>( &ifr ) ) );
}

void assign_address( const string & device_name, const Address & addr, const Address & peer )
{
    Socket ioctl_socket( UDP );

    /* assign address */
    interface_ioctl( ioctl_socket.fd(), SIOCSIFADDR, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = addr.raw_sockaddr(); } );

    /* destination */
    interface_ioctl( ioctl_socket.fd(), SIOCSIFDSTADDR, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = peer.raw_sockaddr(); } );

    /* netmask */
    interface_ioctl( ioctl_socket.fd(), SIOCSIFNETMASK, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_netmask = Address( "255.255.255.255", 0 ).raw_sockaddr(); } );

    /* bring interface up */
    interface_ioctl( ioctl_socket.fd(), SIOCSIFFLAGS, device_name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP | IFF_NOARP; } );
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
}

VirtualEthernetPair::~VirtualEthernetPair()
{
    run( { IP, "link", "del", name } );
    /* deleting one is sufficient to delete both */
}
