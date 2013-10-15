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
        cerr << "Usage: " << argv[ 0 ] << " IP_ADDRESS_TO_BIND" << "INTERFACE OF PROXY" << endl;
        return EXIT_FAILURE;
    }

    /* set up dnat to direct all non-dns TCP traffic from ingress */
    DNAT proxy_test (Address( string( argv[ 1 ] ), 3333 ), string( argv[ 2 ] ) );
    Socket listener_socket( SocketType::TCP );

    /* bind to egress ip and port 3333 */
    listener_socket.bind( Address( string( argv[ 1 ] ), 3333 ) );

    /* listen on listener socket */
    listener_socket.listen();

    /* accept any connection request and then service all GETs */
    while ( true ) {
        thread newthread( [&] ( Socket server_socket ) {
        try {
            /* get original destination for connection request */
            Address original_dst = server_socket.original_dest();
            cout << "connection intended for: " << original_dst.ip() << endl;

            /* create socket and connect to original destination and send original request */
            Socket original( SocketType::TCP );
            original.connect( original_dst );
            original.write( server_socket.read() );

            /* poll on original connect socket and new connection socket to ferry packets */
            struct pollfd pollfds[ 2 ];
	    /* socket connected to original destination */
            pollfds[ 0 ].fd = original.raw_fd();
            pollfds[ 0 ].events = POLLIN;
            /* socket connected to original source */
            pollfds[ 1 ].fd = server_socket.raw_fd();
            pollfds[ 1 ].events = POLLIN;

            /* ferry packets between original source and original destination */
            while(true) {
                if ( poll( pollfds, 2, 60000 ) < 0 ) {
                    throw Exception( "poll" );
                }

                if ( pollfds[ 0 ].revents & POLLIN ) {
                    string buffer = original.read();
                    if ( buffer.empty() ) { break; } /* EOF */
                    server_socket.write( buffer );
                } else if ( pollfds[ 1 ].revents & POLLIN ) {
                    string buffer = server_socket.read();
                    if ( buffer.empty() ) { break; } /* EOF */
                    original.write( buffer );
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
