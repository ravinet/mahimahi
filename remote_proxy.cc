/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include "util.hh"
#include "process_recorder.hh"
#include "process_recorder.cc"
#include "exception.hh"
#include "event_loop.hh"
#include "socket.hh"
#include "address.hh"
#include "poller.hh"
#include "system_runner.hh"
#include "config.h"
#include "phantomjs_configuration.hh"
#include "http_memory_store.hh"
#include "length_value_parser.hh"

using namespace std;
using namespace PollerShortNames;

void handle_client( Socket && client, const int & veth_counter )
{
    LengthValueParser bulk_request_parser;

    Poller poller;

    ProcessRecorder<HTTPProxy<HTTPMemoryStore>> process_recorder;

    bool request_ready = false;

    MahimahiProtobufs::BulkRequest incoming_request;

    poller.add_action( Poller::Action( client, Direction::In,
                                       [&] () {
                                           string buffer = client.read();
                                           auto res = bulk_request_parser.parse( buffer );
                                           if ( res.first ) { /* request protobuf ready */
                                               incoming_request.ParseFromString( res.second );
                                               request_ready = true;
                                           }
                                           return ResultType::Continue;
                                       },
                                       [&] () { return true; } ) );

    poller.add_action( Poller::Action( client, Direction::Out,
                                       [&] () {
                                           /* Obtain url from BulkRequest protobuf */
                                           string scheme = ( incoming_request.scheme() == MahimahiProtobufs::BulkRequest_Scheme_HTTPS
                                                             ? "https" : "http" );
                                           MahimahiProtobufs::HTTPMessage request_message = incoming_request.request();
                                           HTTPRequest curr_request( incoming_request.request() );
                                           string hostname = curr_request.get_header_value( "Host" );
                                           auto path_start = request_message.first_line().find( "/" );
                                           auto path_end = request_message.first_line().find( " ", path_start );
                                           string path = request_message.first_line().substr( path_start, path_end - path_start );
                                           string url = scheme + "://" + hostname + path;


                                           /* Get relevant headers so phantomjs mimics user agent */
                                           string user_agent = "Chrome/31.0.1650.63 Safari/537.36';\n";
                                           string accept = "page.customHeaders = {\"accept\": \"text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\"};";
                                           if ( curr_request.has_header( "User-Agent" ) ) {
                                               user_agent = curr_request.get_header_value( "User-Agent" ) + "';\n";
                                           }

                                           if ( curr_request.has_header( "Accept" ) ) {
                                               accept = "page.customHeaders = {\"accept\": \"" + curr_request.get_header_value( "Accept" ) + "\"};";
                                           }

                                           process_recorder.record_process( []( FileDescriptor & parent_channel ) {
                                                                            SystemCall( "dup2", dup2( parent_channel.num(), STDIN_FILENO ) );
                                                                            return ezexec( { PHANTOMJS, "--ignore-ssl-errors=true",
                                                                                             "--ssl-protocol=TLSv1", "/dev/stdin" } );
                                                                            },
                                                                            move( client ),
                                                                            veth_counter,
                                                                            "url = \"" + url + phantomjs_setup + user_agent + accept + phantomjs_load );
                                           client.write( "" );
                                           request_ready = false;
                                           hostname = "";
                                           return Result::Type::Exit;
                                       },
                                       [&] () { return request_ready; } ) );
    while ( true ) {
        if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
            return;
        }
    }
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " port" );
        }

        /* Use this as part of veth interface name because pid is same for different clients */
        int veth_counter = 0;

        Socket listener_socket( TCP );
        listener_socket.bind( Address( "0", argv[ 1 ] ) );
        listener_socket.listen();

        EventLoop event_loop;

        /* listen for new clients */
        event_loop.add_simple_input_handler( listener_socket,
                                             [&] () {
                                                 Socket client = listener_socket.accept();
                                                 event_loop.add_child_process( ChildProcess( "new_client", [&] () {
                                                         handle_client( move( client ), veth_counter );
                                                         return EXIT_SUCCESS;
                                                     } ), false );
                                                 veth_counter++;
                                                 return ResultType::Continue;
                                             } );
        return event_loop.loop();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
