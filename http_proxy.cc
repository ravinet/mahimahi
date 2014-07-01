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
#include "timestamp.hh"
#include "system_runner.hh"
#include "config.h"
#include "signalfd.hh"
#include "http_proxy.hh"
#include "poller.hh"
#include "bytestream_queue.hh"
#include "http_request_parser.hh"
#include "http_response_parser.hh"
#include "file_descriptor.hh"

using namespace std;
using namespace PollerShortNames;

HTTPProxy::HTTPProxy( const Address & listener_addr, const string & record_folder )
    : listener_socket_( TCP ),
      record_folder_( record_folder)
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

                HTTP_Record::reqrespair current_pair;

                auto dst_port = original_destaddr.port();

                /* Set destination ip, port and protocol in current request/response pair */
                current_pair.set_ip( original_destaddr.ip() );
                current_pair.set_port( dst_port );
                ( dst_port == 443 ) ? current_pair.set_protocol( "HTTPS" ) : current_pair.set_protocol( "HTTP" );

                /* Create Read/Write Interfaces for server and client */
                std::unique_ptr<ReadWriteInterface> server_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( server_rw )>( new SecureSocket( move( server ), CLIENT ) ) :
                                                                 static_cast<decltype( server_rw )>( new Socket( move( server ) ) );
                std::unique_ptr<ReadWriteInterface> client_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( client_rw )>( new SecureSocket( move( client ), SERVER ) ) :
                                                                 static_cast<decltype( client_rw )>( new Socket( move( client ) ) );

                /* poll on original connect socket and new connection socket to ferry packets */
                /* responses from server go to response parser */
                poller.add_action( Poller::Action( server_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = server_rw->read();
                                                       response_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not client_rw->fd().eof(); } ) );

                /* requests from client go to request parser */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = client_rw->read();
                                                       request_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not server_rw->fd().eof(); } ) );

                /* completed requests from client are serialized and sent to server */
                poller.add_action( Poller::Action( server_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       server_rw->write( request_parser.front().str() );
                                                       response_parser.new_request_arrived( request_parser.front() );

                                                       /* add request to current request/response pair */
                                                       current_pair.mutable_req()->CopyFrom( request_parser.front().toprotobuf() );

                                                       request_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not request_parser.empty(); } ) );

                /* completed responses from server are serialized and sent to client */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       client_rw->write( response_parser.front().str() );
                                                       reqres_to_protobuf( current_pair, response_parser.front() );
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

void HTTPProxy::reqres_to_protobuf( HTTP_Record::reqrespair & current_pair, const HTTPResponse & response )
{
    /* output string for current request/response pair */
    string outputmessage;

    /* Use random number generator to create output filename (number between 0 and 99999) */
    string filename = record_folder_ + to_string( random() );

    /* FileDescriptor for output file to write current request/response pair protobuf (user has all permissions) */
    FileDescriptor messages( SystemCall( "open request/response protobuf " + filename + "\n", open(filename.c_str(), O_WRONLY | O_CREAT, 00700 ) ) );

    /* if request is present in current request/response pair, add response and write to file */
    if ( current_pair.has_req() ) {
        current_pair.mutable_res()->CopyFrom( response.toprotobuf() );
        if ( current_pair.SerializeToString( &outputmessage ) ) {
            messages.write( outputmessage );
        } else {
            throw Exception( "Protobuf", "Unable to serialize protobuf to string" );
        }
    } else {
        throw Exception( "Protobuf", "Response ready without Request" );
    }

    /* clear current request/response pair */
    current_pair.clear_req();
    current_pair.clear_res();
}
