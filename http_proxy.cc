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
#include "http_proxy.hh"

using namespace std;

HTTPProxy::HTTPProxy( const Address & listener_addr )
    : listener_socket_( TCP )
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();
}

void HTTPProxy::handle_tcp_get( void )
{
    thread newthread( [&] ( Socket original_source ) {
            try {
                /* get original destination for connection request */
                Address original_destaddr = original_source.original_dest();
                cout << "connection intended for: " << original_destaddr.ip() << endl;

                /* create socket and connect to original destination and send original request */
                Socket original_destination( TCP );
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
                e.perror();
                return;
            }
        }, listener_socket_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}
