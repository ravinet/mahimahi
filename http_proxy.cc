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
                Socket server( TCP );
                server.connect( original_destaddr );

                Poller poller;

                HTTPRequestParser request_parser;

                HTTPResponseParser response_parser;

                /* poll on original connect socket and new connection socket to ferry packets */

                /* responses from server go to response parser */
                poller.add_action( Poller::Action( server.fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = server.read();
                                                       response_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not client.fd().eof(); } ) );

                /* requests from client go to request parser */
                poller.add_action( Poller::Action( client.fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = client.read();
                                                       request_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not server.fd().eof(); } ) );

                /* completed requests from client are serialized and sent to server */
                poller.add_action( Poller::Action( server.fd(), Direction::Out,
                                                   [&] () {
                                                       server.write( request_parser.front().str() );
                                                       response_parser.new_request_arrived( request_parser.front() );
                                                       request_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not request_parser.empty(); } ) );

                /* completed responses from server are serialized and sent to client */
                poller.add_action( Poller::Action( client.fd(), Direction::Out,
                                                   [&] () {
                                                       client.write( response_parser.front().str() );
                                                       response_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not response_parser.empty(); } ) );

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
