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

using namespace std;

void service_request( const Socket & server_socket )
{
    try {
        /* open socket to original destination
        Socket outgoing_socket;
        outgoing_socket.connect( Address( "localhost", "domain" ) ); */

	struct sockaddr_in dstaddr;
        socklen_t destlen = sizeof(dstaddr);
        if (getsockopt( server_socket.raw_fd(), SOL_IP, SO_ORIGINAL_DST, &dstaddr, &destlen ) < 0) {
            throw Exception( "getsockopt" );
        }
        struct pollfd pollfds[ 1 ];
        pollfds[ 0 ].fd = server_socket.raw_fd();
        pollfds[ 0 ].events = POLLIN;

        if ( poll( pollfds, 1, 10000 ) < 0 ) {
            throw Exception( "poll" );
        }

        if ( pollfds[ 0 ].revents & POLLIN ) {
            cout << "packet originally sent to: " << inet_ntoa( dstaddr.sin_addr ) << endl;
        }


        /* poll on server_socket
        struct pollfd pollfds[ 2 ];
        pollfds[ 0 ].fd = outgoing_socket.raw_fd();
        pollfds[ 0 ].events = POLLIN;

        pollfds[ 1 ].fd = server_socket.raw_fd();
        pollfds[ 1 ].events = POLLIN;

        // process requests until either socket is idle for 20 seconds or until EOF
        bool outgoing_eof = false;
        bool server_eof = false;
        while( true ) {
            if ( outgoing_eof || server_eof ) {
                break;
            }
            pollfds[ 0 ].events = outgoing_eof ? 0 : POLLIN;
            pollfds[ 1 ].events = server_eof ? 0 : POLLIN;

             if ( poll( pollfds, 2, 20000 ) < 0 ) {
                throw Exception( "poll" );
            }
            if ( pollfds[ 1 ].revents & POLLIN ) {
                // read request, then send to local dns server
                string buffer = server_socket.read();
                if ( buffer.empty() ) {
                     server_eof = true;
                }
                outgoing_socket.write( buffer );
            }
            if ( poll( &pollfds[ 0 ], 1, 20000 ) < 0 ) {
                throw Exception( "poll" );
            }
           // if response comes from local dns server, write back to source of request
           if ( pollfds[ 0 ].revents & POLLIN ) {
                // read response, then send back to client
                string buffer = outgoing_socket.read();
                if ( buffer.empty() ) {
                     outgoing_eof = true;
                }
                server_socket.write( buffer );
            }
        }
    */
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
    if ( argc != 3 ) {
        cerr << "Usage: " << argv[ 0 ] << " IP_ADDRESS_TO_BIND" << "INTERFACE OF PROXY" << endl;
        return EXIT_FAILURE;
    }

    std::string ip_address_to_bind( argv[ 1 ] );
    std::string ip_port_to_bind(ip_address_to_bind + ":3333");

    std::string interface_to_forward_to( argv[ 2 ] );
    cout << "interface: " << interface_to_forward_to << endl;

    /* add entry to nat table using iptables to direct all TCP traffic to ingress */
    run( { IPTABLES, "-t", "nat", "-A", "PREROUTING", "-p", "TCP", "-i", interface_to_forward_to, "!", "--dport", "53", "-j", "DNAT", "--to-destination", ip_port_to_bind } );

    try {
        Socket listener_socket( SocketType::TCP );
        /* bind to egress ip and port 3333 */
        listener_socket.bind( Address( ip_address_to_bind, 3333 ) );

        /* listen on listener socket */
        listener_socket.listen();
       /* while(1){
            Socket proxy_socket( listener_socket.accept() );
            struct sockaddr_in dstaddr;
            socklen_t destlen = sizeof(dstaddr);
            if (getsockopt( proxy_socket.raw_fd(), SOL_IP, SO_ORIGINAL_DST, &dstaddr, &destlen ) < 0) {
                throw Exception( "getsockopt" );
            }
            struct pollfd pollfds[ 1 ];
            pollfds[ 0 ].fd = proxy_socket.raw_fd();
            pollfds[ 0 ].events = POLLIN;

            if ( poll( pollfds, 1, 50000 ) < 0 ) {
                throw Exception( "poll" );
            }

            if ( pollfds[ 0 ].revents & POLLIN ) {
                cout << "PACKET RECEIVED" << endl;
                cout << "packet originally sent to: " << inet_ntoa( dstaddr.sin_addr ) << endl;
            }

        }*/
	/* accept any connection request and then service all GETs */
        while ( true ) { /* service requests from this source */
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
