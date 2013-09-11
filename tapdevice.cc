#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string>
#include <cstdlib>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "tapdevice.hh"
#include "exception.hh"

using namespace std;

TapDevice::TapDevice( std::string name )
  : fd_()
{
    struct ifreq ifr;
    int err;

    if ( ( fd_ = open( "/dev/net/tun", O_RDWR ) ) < 0 ) {
        throw Exception( "open" );
    }

    // fills enough of memory area pointed to by ifr
    memset( &ifr, 0, sizeof( ifr ) );

    // specifies to create tap device
    ifr.ifr_flags = IFF_TAP;


    // uses specified name for tap device
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ );

    // create interface
    if ( ( err = ioctl( fd_, TUNSETIFF, ( void * ) &ifr ) ) < 0 ){
        close( fd_ );
        throw Exception( "ioctl" );
    }
}

TapDevice::~TapDevice()
{
    if ( close(fd_) < 0 ) {
	throw Exception( "close" );
    }
}
