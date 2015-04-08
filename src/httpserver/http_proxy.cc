/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>

#include "address.hh"
#include "socket.hh"
#include "system_runner.hh"
#include "http_proxy.hh"
#include "poller.hh"
#include "bytestream_queue.hh"
#include "http_request_parser.hh"
#include "http_response_parser.hh"
#include "file_descriptor.hh"
#include "event_loop.hh"
#include "temp_file.hh"
#include "secure_socket.hh"
#include "backing_store.hh"
#include "exception.hh"

using namespace std;
using namespace PollerShortNames;

HTTPProxy::HTTPProxy( const Address & listener_addr )
    : listener_socket_(),
      server_context_( SERVER ),
      client_context_( CLIENT )
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();
}

template <class SocketType>
void HTTPProxy::loop( SocketType & server, SocketType & client, HTTPBackingStore & backing_store )
{
    Poller poller;

    HTTPRequestParser request_parser;
    HTTPResponseParser response_parser;

    const Address server_addr = client.original_dest();

    /* poll on original connect socket and new connection socket to ferry packets */
    /* responses from server go to response parser */
    poller.add_action( Poller::Action( server, Direction::In,
                                       [&] () {
                                           string buffer = server.read();
                                           response_parser.parse( buffer );
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not client.eof(); } ) );

    /* requests from client go to request parser */
    poller.add_action( Poller::Action( client, Direction::In,
                                       [&] () {
                                           string buffer = client.read();
                                           request_parser.parse( buffer );
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not server.eof(); } ) );

    /* completed requests from client are serialized and sent to server */
    poller.add_action( Poller::Action( server, Direction::Out,
                                       [&] () {
                                           server.write( request_parser.front().str() );
                                           response_parser.new_request_arrived( request_parser.front() );
                                           request_parser.pop();
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not request_parser.empty(); } ) );

    /* completed responses from server are serialized and sent to client */
    poller.add_action( Poller::Action( client, Direction::Out,
                                       [&] () {
                                           client.write( response_parser.front().str() );
                                           backing_store.save( response_parser.front(), server_addr );
                                           response_parser.pop();
                                           return ResultType::Continue;
                                       },
                                       [&] () { return not response_parser.empty(); } ) );

    while ( true ) {
        if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
            return;
        }
    }
}

void HTTPProxy::handle_tcp( HTTPBackingStore & backing_store )
{
    thread newthread( [&] ( TCPSocket client ) {
            try {
                /* get original destination for connection request */
                Address server_addr = client.original_dest();

                /* create socket and connect to original destination and send original request */
                TCPSocket server;
                server.connect( server_addr );

                if ( server_addr.port() != 443 ) { /* normal HTTP */
                    return loop( server, client, backing_store );
                }

                /* handle TLS */
                SecureSocket tls_server( client_context_.new_secure_socket( move( server ) ) );
                tls_server.connect();

                SecureSocket tls_client( server_context_.new_secure_socket( move( client ) ) );
                tls_client.accept();

                loop( tls_server, tls_client, backing_store );
            } catch ( const exception & e ) {
                print_exception( e );
            }
        }, listener_socket_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}

/* register this HTTPProxy's TCP listener socket to handle events with
   the given event_loop, saving request-response pairs to the given
   backing_store (which is captured and must continue to persist) */
void HTTPProxy::register_handlers( EventLoop & event_loop, HTTPBackingStore & backing_store )
{
    event_loop.add_simple_input_handler( tcp_listener(),
                                         [&] () {
                                             handle_tcp( backing_store );
                                             return ResultType::Continue;
                                         } );
}
