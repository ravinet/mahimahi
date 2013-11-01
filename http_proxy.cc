/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

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
#include "bytestream_queue.hh"

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

                /* Make bytestream_queue for source->dest and dest->source */
                ByteStreamQueue from_source; ByteStreamQueue from_destination;

                /* poll on original connect socket and new connection socket to ferry packets */
                poller.add_action( Poller::Action( original_destination.fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = original_destination.read();
                                                       if ( buffer.empty() ) { return ResultType::Exit; } /* EOF */
                                                       from_destination.add( buffer );
                                                       return ResultType::Continue;
                                                   } ) );

                poller.add_action( Poller::Action( original_source.fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = original_source.read();
                                                       if ( buffer.empty() ) { return ResultType::Exit; } /* EOF */
                                                       from_source.add( buffer );
                                                       return ResultType::Continue;
                                                   } ) );

                poller.add_action( Poller::Action( original_destination.fd(), Direction::Out,
                                                   [&] () {
                                                       if ( !from_source.empty() ) { /* Bytes to be written */
                                                           size_t amount_written = original_destination.writeval( from_source.head() );
                                                           from_source.update_head( amount_written );
                                                       }
                                                       return ResultType::Continue;
                                                   } ) );

                poller.add_action( Poller::Action( original_source.fd(), Direction::Out,
                                                   [&] () {
                                                       if ( !from_destination.empty() ) { /* Bytes to be written */
                                                           size_t amount_written = original_source.writeval( from_destination.head() );
                                                           from_destination.update_head( amount_written );
                                                       }
                                                       return ResultType::Continue;
                                                   } ) );

                while( true ) {
                    auto poll_result = poller.poll( 60000 );
                    if ( poll_result.result == Poller::Result::Type::Exit ) {
                        return;
                    }
                }
            } catch ( const Exception & e ) {
                e.perror();
                return;
            }
            return;
        }, listener_socket_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}
