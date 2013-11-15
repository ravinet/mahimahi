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
#include "signalfd.hh"
#include "http_proxy.hh"
#include "poller.hh"
#include "bytestream_queue.hh"
#include "http_request_parser.hh"
#include "http_response_parser.hh"

using namespace std;
using namespace PollerShortNames;

HTTPProxy::HTTPProxy( const Address & listener_addr )
    : listener_socket_( TCP )
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();
}

void HTTPProxy::handle_tcp( void )
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

                HTTPRequestParser from_client_parser;

                HTTPResponseParser from_destination_parser;

                /* Make bytestream_queue for source->dest and dest->source */
                ByteStreamQueue from_destination( ezio::read_chunk_size );

                /* poll on original connect socket and new connection socket to ferry packets */

                poller.add_action( Poller::Action( destination.fd(), Direction::In,
                                                   [&] () {
                                                   string buffer = destination.read();
                                                   if ( buffer.empty() ) { return ResultType::Cancel; } /* EOF */
                                                   from_destination_parser.parse( buffer );
                                                   while ( !from_destination_parser.empty() ) {
                                                       client.write( from_destination_parser.get_response().str() );
                                                   }
                                                   return ResultType::Continue; } ) );

                poller.add_action( Poller::Action( client.fd(), Direction::In,
                                                   [&] () {
                                                   string buffer = client.read();
                                                   if ( buffer.empty() ) { return ResultType::Cancel; } /* EOF */
                                                   from_client_parser.parse( buffer );
                                                   while ( not from_client_parser.empty() ) {
                                                       destination.write( from_client_parser.front().str() );
                                                       from_client_parser.pop();
                                                   }
                                                   return ResultType::Continue; } ) );

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
