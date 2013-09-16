/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <poll.h>
#include <queue>
#include <iostream>

#include "ezio.hh"
#include "timestamp.hh"
#include "tapdevice.hh"
#include "exception.hh"
#include "ferry_queue.hh"

using namespace std;

void ferry( TapDevice & ingress_tap, TapDevice & egress_tap, const uint64_t delay_ms )
{
    // set up poll for tap devices
    struct pollfd pollfds[ 2 ];
    pollfds[ 0 ].fd = ingress_tap.fd().num();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = egress_tap.fd().num();
    pollfds[ 1 ].events = POLLIN;

    FerryQueue ingress_queue( delay_ms ), egress_queue( delay_ms );

    while ( true ) {
        int wait_time = min( ingress_queue.wait_time(),
                             egress_queue.wait_time() );

        if( poll( pollfds, 2, wait_time ) == -1 ) {
            throw Exception( "poll" );
        }

        if ( (pollfds[ 0 ].revents & POLLERR) || (pollfds[ 1 ].revents & POLLERR) ) {
            throw Exception( "poll" );
        }

        if ( pollfds[ 0 ].revents & POLLIN ) { /* data on ingress */
            ingress_queue.read_packet( ingress_tap.fd().read() );
        }

        if ( pollfds[ 1 ].revents & POLLIN ) { /* data on egress */
            egress_queue.read_packet( egress_tap.fd().read() );
        }

        ingress_queue.write_packets( egress_tap.fd() );
        egress_queue.write_packets( ingress_tap.fd() );
    }
}

int main ( void )
{
    try {
        // create two tap devices: ingress and egress
        TapDevice ingress_tap( "ingress", "10.0.0.2" );
        TapDevice egress_tap( "egress", "10.0.0.1" );

        ferry( ingress_tap, egress_tap, 2500 );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
