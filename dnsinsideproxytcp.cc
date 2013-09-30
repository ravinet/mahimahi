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
        /* open socket to 172.30.100.100 and an assigned port */
        Socket::protocol prot = Socket::TCP;
        Address::protocol prot_addr = Address::TCP;
        Socket outgoing_socket( prot );
        outgoing_socket.connect( Address( "172.30.100.100", "0", prot_addr ) );

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
                /* read and see if request or eof */
                string buffer = server_socket.read();
                if ( buffer.empty() ) {
                     server_eof = true;
                }
                outgoing_socket.write( buffer );
            }
            if ( pollfds[ 0 ].revents & POLLIN ) {
                /* read response and see if request or eof */
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

int main( void )
{
    /* make listener socket for dns requests */
    try {
        Socket::protocol prot = Socket::TCP;
        Socket listener_socket( prot );
        Address::protocol prot_addr = Address::TCP;
        /* bind to 127.0.0.1 and port 53 for DNS requests */
        listener_socket.bind( Address( "localhost", "domain", prot_addr ) );

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
