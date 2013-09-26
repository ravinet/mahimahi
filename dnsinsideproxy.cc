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
    try {
        /* open socket to egress's ip address and port assigned to outside proxy's listener socket (for now, get assigned port) */
        Socket outgoing_socket;
        outgoing_socket.connect( Address( "172.30.100.100", "0" ) );

        /* send request to local dns server */
        outgoing_socket.write( request.second );

        /* wait up to 20 seconds for a reply */
        struct pollfd pollfds;
        pollfds.fd = outgoing_socket.raw_fd();
        pollfds.events = POLLIN;

        if ( poll( &pollfds, 1, 20000 ) < 0 ) {
            throw Exception( "poll" );
        }

        if ( pollfds.revents & POLLIN ) {
            /* read response, then send back to client */
            server_socket.sendto( request.first, outgoing_socket.read() );
        }
    } catch ( const Exception & e ) {
        cerr.flush();
        e.perror();
        return;
    }        

    cerr.flush();
    return;
}

int main( )
{
    /* make listener socket for dns requests */
    try {
        Socket listener_socket;
        /* bind to 127.0.0.1 and port 53 for DNS requests */
        listener_socket.bind( Address( "localhost", "domain" ) );

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
