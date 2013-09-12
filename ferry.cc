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
#include "timestamp.hh"
#include "tapdevice.hh"

using namespace std;

int main ( void )
{
    // create two tap devices: ingress and egress
    TapDevice ingress_tap( "ingress" );
    TapDevice egress_tap( "egress" );

    // set up poll for tap devices
    struct pollfd pollfds[ 2 ];
    pollfds[ 0 ].fd = ingress_tap.fd();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = egress_tap.fd();
    pollfds[ 1 ].events = POLLIN;

    // amount to delay each packet before forwarding
    uint64_t delay_ms = 2000;
    
    // queue of packets not yet forwarded
    queue<Packet> ingress_queue;
    queue<Packet> egress_queue;

    // poll on both tap devices
    while ( 1 ) {
       // set poll wait time for each queue to time before head packet must be sent
       uint64_t ingress_wait = ingress_queue.empty() ? UINT64_MAX : ingress_queue.front().get_timestamp() - timestamp();
       uint64_t egress_wait = egress_queue.empty() ? UINT64_MAX : egress_queue.front().get_timestamp() - timestamp();
       int wait_time = static_cast<int>(std::min(ingress_wait, egress_wait));
        if( poll( pollfds, 2, wait_time ) == -1 ) {
            perror( "poll" );
            return EXIT_FAILURE;
        }
        // read from ingress which triggered POLLIN
        if ( pollfds[ 0 ].revents & POLLIN ) {
            //string buffer = readall( pollfds[ 0 ].fd );
            string buffer = ingress_tap.read();
            Packet rcvd;
            rcvd.contents = buffer;

            // set the timestamp value to time it should be forwarded
            rcvd.timestamp = timestamp() + delay_ms;
            ingress_queue.push(rcvd);
        }

        // read from egress which triggered POLLIN
        if ( pollfds[ 1 ].revents & POLLIN ) {
            //string buffer = readall( pollfds[ 1 ].fd );
            string buffer = egress_tap.read();
            Packet rcvd;
            rcvd.contents = buffer;

            // set the timestamp value to time it should be forwarded
            rcvd.timestamp = timestamp() + delay_ms;
            egress_queue.push(rcvd);
        }

        // if packet in ingress queue and front ingress packet timestamp matches or is before current time, forward to egress
        if ( !ingress_queue.empty() && ingress_queue.front().get_timestamp() <= timestamp() ) {
            egress_tap.write(ingress_queue.front().get_content());
            ingress_queue.pop();
        }

        // if packet in egress queue and front egress packet timestamp matches or is before current time, forward to ingress
        if ( !egress_queue.empty() && egress_queue.front().get_timestamp() <= timestamp() ) {
            ingress_tap.write(egress_queue.front().get_content());
            egress_queue.pop();
        }
    }
}
