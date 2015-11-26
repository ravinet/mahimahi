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
    : FileDescriptor( SystemCall( "open /dev/net/tun", open( "/dev/net/tun", O_RDWR ) ) )
{
    interface_ioctl( *this, TUNSETIFF, name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_TUN; } );

    assign_address( name, addr, peer );
}

void interface_ioctl( FileDescriptor & fd, const unsigned long request,
                      const string & name,
                      function<void( ifreq &ifr )> ifr_adjustment)
{
    ifreq ifr;
    zero( ifr );
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ ); /* interface name */

    ifr_adjustment( ifr );

    SystemCall( "ioctl " + name, ioctl( fd.fd_num(), request, static_cast<void *>( &ifr ) ) );
}

void interface_ioctl( const unsigned long request,
                      const string & name,
                      function<void( ifreq &ifr )> ifr_adjustment)
{
    /* use a temporary socket */
    UDPSocket temp;
    interface_ioctl( temp, request, name, ifr_adjustment );
}

void assign_address( const string & device_name, const Address & addr, const Address & peer )
{
    /* assign address */
    interface_ioctl( SIOCSIFADDR, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = addr.to_sockaddr(); } );

    /* destination */
    interface_ioctl( SIOCSIFDSTADDR, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = peer.to_sockaddr(); } );

    /* netmask */
    interface_ioctl( SIOCSIFNETMASK, device_name,
                     [&] ( ifreq &ifr )
                     { ifr.ifr_netmask = Address( "255.255.255.255", 0 ).to_sockaddr(); } );

    /* bring interface up */
    interface_ioctl( SIOCSIFFLAGS, device_name,
                     [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
}

void name_check( const string & str )
{
    if ( str.find( "veth-" ) != 0 ) {
        throw runtime_error( str + ": name of veth device must start with \"veth-\"" );
    }
}

VirtualEthernetPair::VirtualEthernetPair( const string & outside_name, const string & inside_name )
    : name_( outside_name ),
      kernel_will_destroy_( false )
{
    /* make pair of veth devices */
    name_check( outside_name );
    name_check( inside_name );

    run( { IP, "link", "add", outside_name, "type", "veth", "peer", "name", inside_name } );
}

VirtualEthernetPair::~VirtualEthernetPair()
{
    if ( kernel_will_destroy_ ) {
        return;
    }

    try {
        run( { IP, "link", "del", name_ } );
    } catch ( const std::exception & e ) {
        print_exception( e );
    }
    /* deleting one is sufficient to delete both */
}
