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
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

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

                Poller poller;

                /* poll on original connect socket and new connection socket to ferry packets */
                poller.add_action( Poller::Action( original_destination.fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = original_destination.read();
                                                       if ( buffer.empty() ) { return ResultType::Continue; } /* EOF */
                                                       original_source.write( buffer );
                                                       return ResultType::Continue;
                                                   } ) );

                poller.add_action( Poller::Action( original_source.fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = original_source.read();
                                                       if ( buffer.empty() ) { return ResultType::Continue; } /* EOF */
                                                       original_destination.write( buffer );
                                                       return ResultType::Continue;
                                                   } ) );

                while( true ) {
                    auto poll_result = poller.poll( 60000 );
                    if ( poll_result.result == Poller::Result::Type::Exit ) {
                        return static_cast<int>( poll_result.exit_status );
                    }
                }
            } catch ( const Exception & e ) {
                e.perror();
                return EXIT_FAILURE;
            }
        }, listener_socket_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}
