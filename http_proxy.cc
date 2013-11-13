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
#include "http_parser.hh"
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

                HTTPParser from_client_parser;

                HTTPResponseParser from_destination_parser;

                /* Make bytestream_queue for source->dest and dest->source */
                ByteStreamQueue from_destination( ezio::read_chunk_size );

                /* poll on original connect socket and new connection socket to ferry packets */

                //string testbuf = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: text/html\r\n\r\nc\r\n<h1>go!</h1>\r\n1b\r\n<h1>first chunk loaded</h1>\r\n2a\r\n<h1>second chunk loaded and displayed</h1>\r\n29\r\n<h1>third chunk loaded and displayed</h1>\r\n0\r\n\r\n";

                poller.add_action( Poller::Action( destination.fd(), Direction::In,
                                                   [&] () {
                                                   string buffer = destination.read();
                                                   if ( buffer.empty() ) { return ResultType::Cancel; } /* EOF */
                                                   from_destination_parser.parse( buffer );
                                                   if ( !from_destination_parser.empty() ) {
                                                       client.write( from_destination_parser.get_response().str() );
                                                   }
                                                   //client.write( buffer );
                                                   return ResultType::Continue; } ) );

                poller.add_action( Poller::Action( client.fd(), Direction::In,
                                                   [&] () {
                                                   string buffer = client.read();
                                                   if ( buffer.empty() ) { return ResultType::Cancel; } /* EOF */
                                                   from_client_parser.parse( buffer );
                                                   if ( !from_client_parser.empty() ) {
                                                       destination.write( from_client_parser.get_request().str() );
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
