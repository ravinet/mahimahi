/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <string>
#include <iostream>
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "address.hh"
#include "socket.hh"
#include "system_runner.hh"
#include "config.h"
#include "signalfd.hh"
#include "http_proxy.hh"
#include "poller.hh"
#include "http_response_parser.hh"
#include "file_descriptor.hh"
#include "archive.hh"

using namespace std;
using namespace PollerShortNames;

HTTPProxy::HTTPProxy( const Address & listener_addr )
    : listener_socket_( TCP )
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();

    /* set starting seed for random number file names */
    srandom( time( NULL ) );

    /* SSL initialization: Needs to be done exactly once */
    /* load algorithms/ciphers */
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    /* load error messages */
    SSL_load_error_strings();
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

                int already_sent = 0;

                auto dst_port = original_destaddr.port();

                bool bulk_thread = false;

                /* Create Read/Write Interfaces for server and client */
                std::unique_ptr<ReadWriteInterface> server_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( server_rw )>( new SecureSocket( move( server ), CLIENT ) ) :
                                                                 static_cast<decltype( server_rw )>( new Socket( move( server ) ) );
                std::unique_ptr<ReadWriteInterface> client_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( client_rw )>( new SecureSocket( move( client ), SERVER ) ) :
                                                                 static_cast<decltype( client_rw )>( new Socket( move( client ) ) );
                /* Make bytestream_queue for browser->server and server->browser */
                ByteStreamQueue from_destination( ezio::read_chunk_size );

                /* poll on original connect socket and new connection socket to ferry packets */
                /* responses from server go to response parser */
                poller.add_action( Poller::Action( server_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = server_rw->read();
                                                       response_parser.parse( buffer, from_destination );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return ( not client_rw->fd().eof() ); } ) );

                /* requests from client go to request parser */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = client_rw->read();
                                                       request_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not server_rw->fd().eof(); } ) );

                /* completed requests from client are serialized and handled by archive or sent to server */
                poller.add_action( Poller::Action( server_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       /* check if request is stored: if pending->wait, if response present->send to client, if neither->send request to server */
                                                       HTTP_Record::http_message complete_request = request_parser.front().toprotobuf();
                                                       /* check if request is GET / (will lead to bulk response) */
                                                       if ( complete_request.first_line() == "GET / HTTP/1.1\r\n" ) {
                                                           server_rw->write( request_parser.front().str() );
                                                           bulk_thread = true;
                                                           response_parser.new_request_arrived( request_parser.front() );
                                                           request_parser.pop();
                                                           return ResultType::Continue;
                                                       }
                                                       if ( Archive::request_pending( complete_request ) ) {
                                                           if ( bulk_thread ) { /* we are on thread with initial request so don't wait for pending request now */
                                                               return ResultType::Continue;
                                                           }
                                                           Archive::wait();
                                                           add_to_queue( from_destination, Archive::corresponding_response( complete_request ), already_sent, request_parser );
                                                       } else if ( Archive::have_response( complete_request ) ) { /* corresponding response already stored- send to client */
                                                           add_to_queue( from_destination, Archive::corresponding_response( complete_request ), already_sent, request_parser );
                                                       } else { /* request not listed in archive- send request to server */
                                                           server_rw->write( request_parser.front().str() );
                                                           response_parser.new_request_arrived( request_parser.front() );
                                                           request_parser.pop();
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not request_parser.empty(); } ) );

                poller.add_action( Poller::Action( client_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       if ( from_destination.non_empty() ) {
                                                           if ( dst_port == 443 ) {
                                                               from_destination.pop_ssl( move( client_rw ) );
                                                           } else {
                                                               from_destination.pop( client_rw->fd() );
                                                           }
                                                       }
                                                       if ( not response_parser.empty() ) {
                                                           client_rw->write( response_parser.front().str() );
                                                           response_parser.pop();
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return (from_destination.non_empty() or ( not response_parser.empty() ) ); } ) );

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

void HTTPProxy::add_to_queue( ByteStreamQueue & responses, string res, int & counter, HTTPRequestParser & req_parser )
{
    /* given the counter and message, figure out how much can be added and add to queue and update counter */
    int avail_space = responses.contiguous_space_to_push();
    int left_to_send = res.size() - counter;
    if ( avail_space >= left_to_send ) { /* enough space to send rest of message */
        responses.push_string( res.substr( counter, left_to_send ) );
        counter = 0;
        req_parser.pop();
    } else if (avail_space < left_to_send and avail_space > 0 ) { /* space for some but not all */
        responses.push_string( res.substr( counter, avail_space ) );
        counter = counter + avail_space;
    }
}
