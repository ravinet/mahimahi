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
#include "local_http_proxy.hh"
#include "poller.hh"
#include "local_http_response_parser.hh"
#include "file_descriptor.hh"
#include "archive.hh"
#include "timestamp.hh"

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

void HTTPProxy::add_poller_actions( Poller & poller,
                                    unique_ptr<ReadWriteInterface> & client_rw,
                                    unique_ptr<ReadWriteInterface> & server_rw,
                                    HTTPResponseParser & response_parser,
                                    HTTPRequestParser & request_parser,
                                    bool & bulk_thread,
                                    ByteStreamQueue & from_destination,
                                    int & already_sent )
{
    /* responses from server go to response parser */
    poller.add_action( Poller::Action( server_rw->fd(), Direction::In,
                                       [&] () {
                                           string buffer = server_rw->read();
                                           response_parser.parse( buffer, from_destination );
                                           return ResultType::Continue;
                                       },
                                       [&] () { return ( not client_rw->fd().eof() ); } ) );

    /* completed requests from client are serialized and handled by archive or sent to server */
    poller.add_action( Poller::Action( server_rw->fd(), Direction::Out,
                                                    [&] () {
                                                        HTTP_Record::http_message complete_request = request_parser.front().toprotobuf();
                                                        string first_line = complete_request.first_line();
                                                        /* check if request is GET / (will lead to bulk response) */
                                                        if ( first_line == "GET / HTTP/1.1\r\n" ) {
                                                            cout << "WRITING FIRST REQUEST TO SERVER AT: " << timestamp() << endl;
                                                            server_rw->write( request_parser.front().str() );
                                                            bulk_thread = true;
                                                            response_parser.new_request_arrived( request_parser.front() );
                                                            request_parser.pop();
                                                        } else if ( Archive::request_pending( complete_request ) ) {
                                                            cout << "PENDING REQUEST: " << first_line << " at: " << timestamp() << endl;
                                                            if ( bulk_thread ) { /* don't wait so we can keep parsing bulk response */
                                                                return ResultType::Continue;
                                                            }
                                                            Archive::wait();
                                                            cout << "DONE PENDING: " << first_line << " at: " << timestamp() << endl;
                                                            add_to_queue( from_destination, Archive::corresponding_response( complete_request ),
                                                                          already_sent, request_parser );
                                                        } else if ( Archive::have_response( complete_request ) ) { /* response stored- send to client */
                                                            cout << "REQUEST IN ARCHIVE: " << first_line << " at: " << timestamp() << endl;
                                                            add_to_queue( from_destination, Archive::corresponding_response( complete_request ),
                                                                          already_sent, request_parser );
                                                        } else { /* request not in archive->send request to server */
                                                            cout << "REQUEST NOT IN ARCHIVE: " << first_line << " at: " << timestamp() << endl;
                                                            server_rw->write( request_parser.front().str() );
                                                            response_parser.new_request_arrived( request_parser.front() );
                                                            request_parser.pop();
                                                        }
                                                        return ResultType::Continue;
                                                    },
                                                    [&] () { return not request_parser.empty(); } ) );
}

void HTTPProxy::handle_tcp( void )
{
    thread newthread( [&] ( Socket client ) {
            try {
                /* get original destination for connection request */
                Address original_destaddr = client.original_dest();
                cout << "connection intended for: " << original_destaddr.ip() << endl;

                /* create socket which may later connect to original destination */
                Socket server( TCP );

                Poller poller;

                HTTPRequestParser request_parser;

                HTTPResponseParser response_parser;

                int already_sent = 0;

                auto dst_port = original_destaddr.port();

                bool bulk_thread = false;

                bool connected = false;

                /* Create Read/Write Interfaces for server and client */
                unique_ptr<ReadWriteInterface> server_rw;
                unique_ptr<ReadWriteInterface> client_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( client_rw )>( new SecureSocket( move( client ), SERVER ) ) :
                                                                 static_cast<decltype( client_rw )>( new Socket( move( client ) ) );
                /* Make bytestream_queue for browser->server and server->browser */
                ByteStreamQueue from_destination( ezio::read_chunk_size );

                /* poll on original connect socket and new connection socket to ferry packets */
                /* requests from client go to request parser */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = client_rw->read();
                                                       request_parser.parse( buffer );
                                                       if ( buffer.find( "GET" ) != string::npos) { cout << "INCOMING REQUEST AT: " << timestamp() << endl;}
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return true; } ) );

                poller.add_action( Poller::Action( client_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       if ( from_destination.non_empty() ) {
                                                           cout << "WRITING RESPONSE TO CLIENT FROM BYTESTREAM AT: " << timestamp() << endl;
                                                           if ( dst_port == 443 ) {
                                                               from_destination.pop_ssl( move( client_rw ) );
                                                           } else {
                                                               from_destination.pop( client_rw->fd() );
                                                           }
                                                       }
                                                       if ( not response_parser.empty() ) {
                                                           cout << "WRITING RESPONSE TO CLIENT FROM RESPONSE PARSER AT: " << timestamp() << endl;
                                                           client_rw->write( response_parser.front().str() );
                                                           response_parser.pop();
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return (from_destination.non_empty() or ( not response_parser.empty() ) ); } ) );

                while( true ) {
                    auto poll_result = poller.poll( 60000 );
                    if ( not connected and not request_parser.empty() ) { /* have complete request and are not connected to server */
                        HTTP_Record::http_message complete_request = request_parser.front().toprotobuf();
                        string first_line = complete_request.first_line();
                        /* check if request is GET / (will lead to bulk response) */
                        if ( first_line == "GET / HTTP/1.1\r\n" ) {
                            cout << "WRITING FIRST REQUEST TO SERVER AT: " << timestamp() << endl;
                            int remote_port = 4567;
                            if ( dst_port == 443 ) { remote_port = 5678; }
                            Address remote_proxy_addr( "54.210.12.57", remote_port );
                            server.connect( remote_proxy_addr );
                            server_rw  = (dst_port == 443) ?
                                         static_cast<decltype( server_rw )>( new SecureSocket( move( server ), CLIENT ) ) :
                                         static_cast<decltype( server_rw )>( new Socket( move( server ) ) );
                            server_rw->write( request_parser.front().str() );
                            bulk_thread = true;
                            response_parser.new_request_arrived( request_parser.front() );
                            request_parser.pop();
                            add_poller_actions( poller, client_rw, server_rw, response_parser,
                                                request_parser, bulk_thread, from_destination, already_sent );
                            /* We have connected to the server and added poller actions for that */
                            connected = true;
                        } else if ( Archive::request_pending( complete_request ) ) { /* request pending->wait */
                            assert ( not bulk_thread ); /* should never be on bulk thread here b/c we would have already connected */
                            cout << "PENDING REQUEST: " << first_line << " at: " << timestamp() << endl;
                            Archive::wait();
                            cout << "DONE PENDING: " << first_line << " at: " << timestamp() << endl;
                            add_to_queue( from_destination, Archive::corresponding_response( complete_request ),
                                          already_sent, request_parser );
                        } else if ( Archive::have_response( complete_request ) ) { /* response stored- send to client */
                            cout << "REQUEST IN ARCHIVE: " << first_line << " at: " << timestamp() << endl;
                            add_to_queue( from_destination, Archive::corresponding_response( complete_request ),
                                          already_sent, request_parser );
                        } else { /* request not in archive->send request to server */
                            cout << "REQUEST NOT IN ARCHIVE: " << first_line << " at: " << timestamp() << endl;
                            int remote_port = 4567;
                            if ( dst_port == 443 ) { remote_port = 5678; }
                            Address remote_proxy_addr( "54.210.12.57", remote_port );
                            server.connect( remote_proxy_addr );
                            server_rw  = (dst_port == 443) ?
                                         static_cast<decltype( server_rw )>( new SecureSocket( move( server ), CLIENT ) ) :
                                         static_cast<decltype( server_rw )>( new Socket( move( server ) ) );
                            server_rw->write( request_parser.front().str() );
                            response_parser.new_request_arrived( request_parser.front() );
                            request_parser.pop();
                            add_poller_actions( poller, client_rw, server_rw, response_parser,
                                                request_parser, bulk_thread, from_destination, already_sent );
                            connected = true;
                        }
                    }
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
