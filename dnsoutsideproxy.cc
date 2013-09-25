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

void service_request( const Socket & server_socket, const pair< Address, std::string > request )
{
    /* open socket to 127.0.0.1:53 */
    Socket outgoing_socket;
    outgoing_socket.bind( Address( "localhost", "domain" ) );

    /* send request to local dns server */
    outgoing_socket.write( request.second );

    /* wait up to 10 seconds for a reply */
    struct pollfd pollfds;
    pollfds.fd = outgoing_socket.raw_fd();
    pollfds.events = POLLIN;

    if ( poll( &pollfds, 1, 10000 ) < 0 ) {
        throw Exception( "poll" );
    }

    if ( pollfds.revents & POLLIN ) {
        /* read response, then send back to client */
        server_socket.sendto( request.first, outgoing_socket.read() );
    } else if ( pollfds.revents & POLLERR ) {
        cerr << "POLLERR" << endl;
    } else {
        cerr << "Timeout." << endl;
    }

    cerr.flush();

    return;
}

int main( void )
{
    /* make listener socket for dns requests */
    try {
        Socket listener_socket;
        /* bind to any IP address and kernel-allocated port */
        listener_socket.bind( Address() );

        cout << "hostname: " << listener_socket.local_addr().hostname() << endl;
        cout << "port: " << listener_socket.local_addr().port() << endl;

        while ( true ) {  /* service a request */
            thread newthread( [&listener_socket] ( const pair< Address, string > request ) -> void {
                    service_request( listener_socket, request ); },
                listener_socket.recvfrom() );
            newthread.detach();
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
