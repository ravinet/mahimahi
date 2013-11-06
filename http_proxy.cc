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
    thread newthread( [&] ( Socket client ) {
            try {
                /* get original destination for connection request */
                Address original_destaddr = client.original_dest();
                cout << "connection intended for: " << original_destaddr.ip() << endl;

                /* create socket and connect to original destination and send original request */
                Socket destination( TCP );
                destination.connect( original_destaddr );

                Poller poller;

                /* Make bytestream_queue for source->dest and dest->source */
                ByteStreamQueue from_client( ezio::read_chunk_size ); ByteStreamQueue from_destination( ezio::read_chunk_size );

                /* poll on original connect socket and new connection socket to ferry packets */

                poller.add_action( Poller::Action( destination.fd(), Direction::In,
                                                   [&] () { return eof( from_destination.push( destination.fd() ) ) ? ResultType::Exit : ResultType::Continue; },
                                                   from_destination.space_available ) );

                poller.add_action( Poller::Action( client.fd(), Direction::In,
                                                   [&] () { return eof( from_client.push( client.fd() ) ) ? ResultType::Exit : ResultType::Continue; },
                                                   from_client.space_available ) );

                poller.add_action( Poller::Action( destination.fd(), Direction::Out,
                                                   [&] () { from_client.pop( destination.fd() ); return ResultType::Continue; },
                                                   from_client.non_empty ) );

                poller.add_action( Poller::Action( client.fd(), Direction::Out,
                                                   [&] () { from_destination.pop( client.fd() ); return ResultType::Continue; },
                                                   from_destination.non_empty ) );

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
