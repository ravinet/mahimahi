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

void ferry( TapDevice & src_tap, TapDevice & dest_tap, const uint64_t delay_ms )
{
    // set up poll for tap devices
    struct pollfd pollfds[ 1 ];
    pollfds[ 0 ].fd = src_tap.fd();
    pollfds[ 0 ].events = POLLIN;

    FerryQueue packet_queue( delay_ms );

    while ( true ) {
        int wait_time = packet_queue.wait_time();
        
        if( poll( pollfds, 1, wait_time ) == -1 ) {
            throw Exception( "poll" );
        }

        if ( pollfds[ 0 ].revents & POLLERR ) { /* check for error */
            throw Exception( "poll" );
        }

        if ( pollfds[ 0 ].revents & POLLIN ) { /* data on src */
            packet_queue.read_packet( src_tap.read() );
        }

        packet_queue.write_packets( dest_tap );
    }
}

int main( void )
{
    try {
        // create two tap devices: ingress and egress
        TapDevice ingress_tap( "ingress" );
        TapDevice egress_tap( "egress" );

        /* Fork */
        pid_t child_pid = fork();
        if ( child_pid < 0 ) {
            throw Exception( "fork" );
        }

        if ( child_pid == 0 ) { /* child */
            ferry( ingress_tap, egress_tap, 2500 );
        } else { /* parent */
            ferry( egress_tap, ingress_tap, 2500 );
        }
    } catch ( const Exception & e ) {
        e.die();
    }

    return EXIT_SUCCESS;
}
