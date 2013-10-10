/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>

#include <thread>
#include <string>
#include <iostream>
#include <utility>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "system_runner.hh"

using namespace std;
/* set up DNAT which, for any tcp packet maps dest addr to ingress:80. Ingress then receives packet and sends the packet via egress and packet will come back to ingress which will know to send to source address */



void service_request( const Socket & server_socket )
{
    try {
        /* GET arrived at ingress...want socket to bind to ingress and connect to original destination address */
        Socket outgoing_socket( SocketType::TCP );
        outgoing_socket.connect( Address( "localhost", "domain", SocketType::TCP ) );

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
    ip_address_to_bind = ip_address_to_bind + ":80";

    /* add entry to nat table using iptables to direct all TCP traffic to ingress */
    run( { IPTABLES, "-t", "nat", "-A", "PREROUTING", "-p", "TCP", "-j", "DNAT", "--to-destination", ip_address_to_bind } );        


    /* make listener socket for dns requests */
    try {
        Socket listener_socket( SocketType::TCP );
        /* bind to ingress ip and port 80 */
        listener_socket.bind( Address( ip_address_to_bind, 80 ) );

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
