// to bring interface for tap device up --> sudo ip link set egress up
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

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
#include "ezio.hh"
#include <time.h>
#include <queue>
#include "packet.hh"

using namespace std;

int create_tap( std::string name )
{
    struct ifreq ifr;
    int fd, err;

    if ( ( fd = open( "/dev/net/tun", O_RDWR ) ) < 0 ) {
        perror( "open" );
        return fd;
    }

    // fills enough of memory area pointed to by ifr
    memset( &ifr, 0, sizeof( ifr ) );

    // specifies to create tap device
    ifr.ifr_flags = IFF_TAP;


    // uses specified name for tap device
    strncpy( ifr.ifr_name, name.c_str(), IFNAMSIZ );

    // create interface
    if ( ( err = ioctl( fd, TUNSETIFF, ( void * ) &ifr ) ) < 0 ){
        close( fd );
        perror( "ioctl" );
        return EXIT_FAILURE;
    }
    return fd;
}

int main ( void )
{
    // create first tap device named ingress
    int tap0_fd = create_tap( "ingress" );

    // create second tap device named egress
    int tap1_fd = create_tap( "egress" );

    // set up poll for tap devices
    struct pollfd pollfds[ 2 ];
    pollfds[ 0 ].fd = tap0_fd;
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = tap1_fd;
    pollfds[ 1 ].events = POLLIN;

    // amount to delay each packet before forwarding
    int delay = 2;

    // queue of packets not yet forwarded
    queue<Packet> ingress_packets;
    queue<Packet> egress_packets;

    // poll on both tap devices
    while ( 1 ) {
        if( poll( pollfds, 2, -1 ) == -1 ) {
            perror( "poll" );
            return EXIT_FAILURE;
        }
        // read from ingress which triggered POLLIN
        if ( pollfds[ 0 ].revents & POLLIN ) {
            string buffer = readall( pollfds[ 0 ].fd );
            Packet newest;
            newest.contents = buffer;

            // set the timestamp value to time it should be forwarded
            newest.timestamp = time(NULL) + delay;
            ingress_packets.push(newest);
            cout << "ingress: " << ingress_packets.size() << endl;
        }

        // read from egress which triggered POLLIN
        if ( pollfds[ 1 ].revents & POLLIN ) {
            string buffer = readall( pollfds[ 1 ].fd );
            Packet newest;
            newest.contents = buffer;

            // set the timestamp value to time it should be forwarded
            newest.timestamp = time(NULL) + delay;
            egress_packets.push(newest);
            cout << "egress: " << egress_packets.size() << endl;
        }

        // if front ingress packet timestamp matches or is before current time, forward to egress
        if ( ingress_packets.front().get_timestamp() <= time(NULL) && ingress_packets.front().get_timestamp() != 0) {
            writeall( pollfds[ 1 ].fd, ingress_packets.front().get_content());
            cout << "off by: " << time(NULL) - ingress_packets.front().get_timestamp() << endl;
            ingress_packets.pop();
        }

         // if front egress packet timestamp matches or is before current time, forward to ingress
        if ( egress_packets.front().get_timestamp() <= time(NULL) && egress_packets.front().get_timestamp() != 0) {
            writeall( pollfds[ 0 ].fd, egress_packets.front().get_content());
            cout << "off by: " << time(NULL) - egress_packets.front().get_timestamp() << endl;
            egress_packets.pop();
        }
    }
}
