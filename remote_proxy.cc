/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include "util.hh"
#include "process_recorder.hh"
#include "exception.hh"
#include "event_loop.hh"
#include "socket.hh"
#include "address.hh"
#include "http_request_parser.hh"
#include "poller.hh"
#include "system_runner.hh"
#include "config.h"
#include "phantomjs_configuration.hh"

using namespace std;
using namespace PollerShortNames;

void handle_client( Socket && client, const int & veth_counter )
{
    HTTPRequestParser request_parser;

    Poller poller;

    ProcessRecorder process_recorder;

    bool done_loading = false;

    poller.add_action( Poller::Action( client, Direction::In,
                                       [&] () {
                                           string buffer = client.read();
                                           request_parser.parse( buffer );
                                           return ResultType::Continue;
                                       },
                                       [&] () { return true; } ) );

    poller.add_action( Poller::Action( client, Direction::Out,
                                       [&] () {
                                           client.write( "DONE" );
                                           done_loading = false;
                                           return Result::Type::Exit;
                                       },
                                       [&] () { return done_loading; } ) );

    while ( true ) {
        if ( not request_parser.empty() )  { /* we have a complete request */
            string url;
            HTTPRequest incoming_request = request_parser.front();
            request_parser.pop();
            if ( incoming_request.has_header( "Host" ) ) {
                url = incoming_request.get_header_value( "Host" );
            }
            process_recorder.record_process( []( FileDescriptor & parent_channel ) {
                                             SystemCall( "dup2", dup2( parent_channel.num(), STDIN_FILENO ) );
                                             return ezexec( { PHANTOMJS, "--ignore-ssl-errors=true",
                                                              "--ssl-protocol=TLSv1", "/dev/stdin" } );
                                             },
                                             veth_counter,
                                             "url = \"" + url + phantomjs_config );
            done_loading = true;
        }
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
