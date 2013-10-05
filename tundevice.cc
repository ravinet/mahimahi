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
    interface_ioctl( fd_.num(), TUNSETIFF, name,
                     [] ( struct ifreq &ifr,
                          struct sockaddr_in & ) { ifr.ifr_flags = IFF_TUN; } );

    Socket sockfd( SocketType::UDP );

    /* bring interface up */
    interface_ioctl( sockfd.raw_fd(), SIOCSIFFLAGS, name,
                     [] ( struct ifreq &ifr,
                          struct sockaddr_in & ) { ifr.ifr_flags = IFF_UP; } );

    /* assign interface address */
    interface_ioctl( sockfd.raw_fd(), SIOCSIFADDR, name,
                     [&] ( struct ifreq &ifr,
                           struct sockaddr_in &sin )
                  {
                      sin = Address( addr, 0 ).raw_sockaddr();
                      ifr.ifr_addr = *reinterpret_cast<sockaddr *>( &sin );
                  } );

    /* assign destination addresses */
    interface_ioctl( sockfd.raw_fd(), SIOCSIFDSTADDR, name,
                     [&] ( struct ifreq &ifr,
                           struct sockaddr_in &sin )
                  {
                      sin = Address( dstaddr, 0 ).raw_sockaddr();
                      ifr.ifr_dstaddr = *reinterpret_cast<sockaddr *>( &sin );
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
    
