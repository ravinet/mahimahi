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
                ByteStreamQueue from_client( 1048576 ); ByteStreamQueue from_destination( 1048576 );

                /* poll on original connect socket and new connection socket to ferry packets */

                poller.add_action( Poller::Action( destination.fd(), Direction::In,
                                                   [&] () {
                                                       /* read up to number of bytes available to write to in bytestreamqueue */
                                                       if ( from_destination.available_to_write() > 0 ) {
                                                           string buffer = destination.read( from_destination.available_to_write() );
                                                           if ( buffer.empty() ) { return ResultType::Exit; } /* EOF */
                                                           from_destination.write( buffer );
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () {
                                                       return from_destination.available_to_write() > 0;
                                                   } ) );

                poller.add_action( Poller::Action( client.fd(), Direction::In,
                                                   [&] () {
                                                       /* read up to number of bytes available to write to in bytestreamqueue */
                                                       if ( from_client.available_to_write() > 0 ) {
                                                           string buffer = client.read( from_client.available_to_write() );
                                                           if ( buffer.empty() ) { return ResultType::Exit; } /* EOF */
                                                           from_client.write( buffer );
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () {
                                                       return from_client.available_to_write() > 0;
                                                   } ) );

                poller.add_action( Poller::Action( destination.fd(), Direction::Out,
                                                   [&] () {
                                                       /* write to Filedescriptor only if bytes available to be read from bytestreamqueue */
                                                       if ( from_client.available_to_read() > 0 ) {
                                                           from_client.write_to_fd( destination.fd() );
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () {
                                                       return from_client.available_to_read() > 0;
                                                   } ) );

                poller.add_action( Poller::Action( client.fd(), Direction::Out,
                                                   [&] () {
                                                       /* write to Filedescriptor only if bytes available to be read from bytestreamqueue */
                                                       if ( from_destination.available_to_read() > 0 ) {
                                                           from_destination.write_to_fd( client.fd() );
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () {
                                                       return from_destination.available_to_read() > 0;
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
