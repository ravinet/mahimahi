/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <string>
#include <iostream>
#include <utility>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"
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

using namespace std;
using namespace PollerShortNames;

HTTPProxy::HTTPProxy( const Address & listener_addr, const string & record_folder )
    : listener_socket_( TCP ),
      record_folder_( record_folder)
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();

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
                //                cout << "connection intended for: " << original_destaddr.ip() << endl;

                /* create socket and connect to original destination and send original request */
                Socket server( TCP );
                server.connect( original_destaddr );

                Poller poller;

                HTTPRequestParser request_parser;
                HTTPResponseParser response_parser;

                /* Create Read/Write Interfaces for server and client */
                std::unique_ptr<ReadWriteInterface> server_rw  = (original_destaddr.port() == 443) ?
                                                                 static_cast<decltype( server_rw )>( new SecureSocket( move( server ), CLIENT ) ) :
                                                                 static_cast<decltype( server_rw )>( new Socket( move( server ) ) );
                std::unique_ptr<ReadWriteInterface> client_rw  = (original_destaddr.port() == 443) ?
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
                                                       request_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not request_parser.empty(); } ) );

                /* completed responses from server are serialized and sent to client */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       client_rw->write( response_parser.front().str() );
                                                       save_to_disk( response_parser.front(), original_destaddr );
                                                       response_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not response_parser.empty(); } ) );

                while( true ) {
                    if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
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

void HTTPProxy::save_to_disk( const HTTPResponse & response, const Address & server_address )
{
    /* output file to write current request/response pair protobuf (user has all permissions) */
    UniqueFile file( record_folder_ + "save" );

    /* construct protocol buffer */
    MahimahiProtobufs::RequestResponse output;

    output.set_ip( server_address.ip() );
    output.set_port( server_address.port() );
    output.set_scheme( server_address.port() == 443
                       ? MahimahiProtobufs::RequestResponse_Scheme_HTTPS
                       : MahimahiProtobufs::RequestResponse_Scheme_HTTP );
    output.mutable_request()->CopyFrom( response.request().toprotobuf() );
    output.mutable_response()->CopyFrom( response.toprotobuf() );

    if ( not output.SerializeToFileDescriptor( file.fd().num() ) ) {
        throw Exception( "save_to_disk", "failure to serialize HTTP request/response pair" );
    }
}

void HTTPProxy::register_handlers( EventLoop & event_loop )
{
    event_loop.add_simple_input_handler( tcp_listener().fd(),
                                         [&] () { handle_tcp(); return ResultType::Continue; } );
}
