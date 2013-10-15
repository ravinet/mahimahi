/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>

#include <thread>
#include <string>
#include <iostream>
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "system_runner.hh"
#include "config.h"
#include "dnat.hh"

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

    /* accept any connection request and then service all GETs */
    while ( true ) {
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
}
