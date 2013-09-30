/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>

#include <thread>
#include <string>
#include <iostream>
#include <utility>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"

using namespace std;

void service_request( const Socket & server_socket )
{
    try {
        /* open socket to 127.0.0.1:53 */
        Socket outgoing_socket;
        outgoing_socket.connect( Address( "localhost", "domain" ) );

        struct pollfd pollfds[ 2 ];
        pollfds[ 0 ].fd = outgoing_socket.raw_fd();
        pollfds[ 0 ].events = POLLIN;
          
        pollfds[ 1 ].fd = server_socket.raw_fd();
        pollfds[ 1 ].events = POLLIN;

        /* process requests until either socket is idle for 20 seconds or until EOF */
        bool outgoing_eof = false;
        bool server_eof   = false;
        while( true ) {
            if ( outgoing_eof || server_eof ) {
                break;
            }
            pollfds[ 0 ].events = outgoing_eof ? 0 : POLLIN;
            pollfds[ 1 ].events = server_eof   ? 0 : POLLIN;

             if ( poll( pollfds, 2, 20000 ) < 0 ) {
                throw Exception( "poll" );
            }
            if ( pollfds[ 1 ].revents & POLLIN ) {
                /* read request, then send to local dns server */
                string buffer = server_socket.read();
                if ( buffer.empty() ) {
                     server_eof = true;
                }
                outgoing_socket.write( buffer );
            }
            if ( poll( &pollfds[ 0 ], 1, 20000 ) < 0 ) {
                throw Exception( "poll" );
            }
           /* if response comes from local dns server, write back to source of request */
           if ( pollfds[ 0 ].revents & POLLIN ) {
                /* read response, then send back to client */
                string buffer = outgoing_socket.read();
                if ( buffer.empty() ) {
                     outgoing_eof = true;
                }
                server_socket.write( buffer );
            }
        }
    
    } catch ( const Exception & e ) {
        cerr.flush();
        e.perror();
        return;
    }        

    cerr.flush();
    return;
}

int main( int argc, char *argv[] )
{
    if ( argc != 2 ) {
        cerr << "Usage: " << argv[ 0 ] << " IP_ADDRESS_TO_BIND" << endl;
        return EXIT_FAILURE;
    }

    std::string ip_address_to_bind( argv[ 1 ] );

    /* make listener socket for dns requests */
    try {
        Socket listener_socket;
        /* bind to egress ip and kernel-allocated port */
        listener_socket.bind( Address( ip_address_to_bind, "0" ) );

        cout << "hostname: " << listener_socket.local_addr().hostname() << endl;
        cout << "port: " << listener_socket.local_addr().port() << endl;

        /* listen on listener socket */
        listener_socket.listen();

        while ( true ) {  /* service requests from this source  */
            thread newthread( [] (const Socket & service_socket) -> void {
                              service_request( service_socket ) ; },
                              listener_socket.accept());
            newthread.detach();
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
