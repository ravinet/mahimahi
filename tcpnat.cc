/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>

#include <thread>
#include <string>
#include <iostream>
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <signal.h>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "system_runner.hh"
#include "config.h"
#include "dnat.hh"
#include "signalfd.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    if ( argc != 3 ) {
        cerr << "Usage: " << argv[ 0 ] << " IP_ADDRESS_TO_BIND" << " INTERFACE OF PROXY" << endl;
        return EXIT_FAILURE;
    }

    /* set up dnat to direct all non-dns TCP traffic from ingress */
    Address listener_addr( string( argv[ 1 ] ), 3333 );
    DNAT http_forwarding( listener_addr, string( argv[ 2 ] ) );
    Socket listener_socket( SocketType::TCP );

    listener_socket.bind( listener_addr );

    listener_socket.listen();

    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGINT };
    signals_to_listen_for.block(); /* don't let them interrupt us */
    SignalFD signal_fd( signals_to_listen_for );

    /* Poll structure */
    pollfd pollfds[ 2 ];

    pollfds[ 0 ].fd = listener_socket.raw_fd();
    pollfds[ 0 ].events = POLLIN;

    pollfds[ 1 ].fd = signal_fd.raw_fd();
    pollfds[ 1 ].events = POLLIN;

    /* accept any connection request and then service all GETs */
    while ( true ) {
        const nfds_t num_pollfds = 2;
        if ( poll( pollfds, num_pollfds, -1 ) == -1 ) {
            throw Exception( "poll" );
        }

        /* check for error on any fd */
        short revents = 0;
        for ( nfds_t i = 0; i < num_pollfds; i++ ) {
            revents |= pollfds[ i ].revents;
        }
        if ( revents & (POLLERR | POLLHUP | POLLNVAL) ) {
            throw Exception( "poll" );
        }

        if (pollfds[ 0 ].revents & POLLIN) {
        thread newthread( [&] ( Socket original_source ) {
        try {
            /* get original destination for connection request */
            Address original_destaddr = original_source.original_dest();
            cout << "connection intended for: " << original_destaddr.ip() << endl;

            /* create socket and connect to original destination and send original request */
            Socket original_destination( SocketType::TCP );
            original_destination.connect( original_destaddr );

            /* poll on original connect socket and new connection socket to ferry packets */
            struct pollfd pollfds[ 2 ];
            pollfds[ 0 ].fd = original_destination.raw_fd();
            pollfds[ 0 ].events = POLLIN;

            pollfds[ 1 ].fd = original_source.raw_fd();
            pollfds[ 1 ].events = POLLIN;

            /* ferry packets between original source and original destination */
            while(true) {
                if ( poll( pollfds, 2, 60000 ) < 0 ) {
                    throw Exception( "poll" );
                }

                if ( pollfds[ 0 ].revents & POLLIN ) {
                    string buffer = original_destination.read();
                    if ( buffer.empty() ) { break; } /* EOF */
                    original_source.write( buffer );
                } else if ( pollfds[ 1 ].revents & POLLIN ) {
                    string buffer = original_source.read();
                    if ( buffer.empty() ) { break; } /* EOF */
                    original_destination.write( buffer );
                } else {
                  break; /* timeout */
                }
            }
        } catch ( const Exception & e ) {
            cerr.flush();
            e.perror();
            return;
        }
        cerr.flush();
        return;
        }, listener_socket.accept() );

        /* don't wait around for the reply */
        newthread.detach();
        }

        if ( pollfds[ 1 ].revents & POLLIN ) {
            /* signal handling */
            printf("Signal handling\n");
            break;
        }
    }
    return EXIT_SUCCESS;
}
