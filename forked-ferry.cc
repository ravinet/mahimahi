/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <poll.h>
#include <queue>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

#include "tapdevice.hh"
#include "exception.hh"
#include "ferry_queue.hh"

using namespace std;

void ferry( const FileDescriptor & tap, const FileDescriptor & sibling_fd, const uint64_t delay_ms )
{
    // set up poll for tap devices
    struct pollfd pollfds[ 2 ];
    pollfds[ 0 ].fd = tap.num();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = sibling_fd.num();
    pollfds[ 1 ].events = POLLIN;

    FerryQueue delay_queue( delay_ms );

    while ( true ) {
        int wait_time = delay_queue.wait_time();
        
        if ( poll( pollfds, 2, wait_time ) == -1 ) {
            throw Exception( "poll" );
        }

        if ( (pollfds[ 0 ].revents | pollfds[ 1 ].revents) & POLLERR ) { /* check for error */
            throw Exception( "poll" );
        }

        if ( pollfds[ 0 ].revents & POLLIN ) {
            /* packet FROM tap device goes to back of delay queue */
            delay_queue.read_packet( tap.read() );
        }

        if ( pollfds[ 1 ].revents & POLLIN ) {
            /* packet FROM sibling goes to tap device */
            tap.write( sibling_fd.read() );
        }

        /* packets FROM tail of delay queue go to sibling */
        delay_queue.write_packets( sibling_fd );
    }
}

int main( void )
{
    try {
        /* make pair of connected sockets */
        int pipes[ 2 ];
        if ( socketpair( AF_UNIX, SOCK_DGRAM, 0, pipes ) < 0 ) {
            throw Exception( "socketpair" );
        }
        FileDescriptor egress_socket( pipes[ 0 ], "socketpair" ),
            ingress_socket( pipes[ 1 ], "socketpair" );

        /* Fork */
        pid_t child_pid = fork();
        if ( child_pid < 0 ) {
            throw Exception( "fork" );
        }

        if ( child_pid == 0 ) { /* child */
            /* unshare here???????? */
            TapDevice ingress_tap( "ingress" );
            ferry( ingress_tap.fd(), egress_socket, 2500 );
        } else { /* parent */
            TapDevice egress_tap( "egress" );
            ferry( egress_tap.fd(), ingress_socket, 2500 );
        }
    } catch ( const Exception & e ) {
        e.die();
    }

    return EXIT_SUCCESS;
}
