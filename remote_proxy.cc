/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include "util.hh"
#include "url_loader.hh"
#include "exception.hh"
#include "event_loop.hh"
#include "socket.hh"
#include "address.hh"
#include "http_request_parser.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

void handle_client( Socket & client, const int & veth_counter )
{
    HTTPRequestParser request_parser;

    Poller poller;

    URLLoader load_page;

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
            load_page.get_all_resources( url, veth_counter );
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

        if ( argc != 1 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) );
        }

        /* Use this as part of veth interface name because pid is same for different clients */
        int veth_counter = 0;

        Socket listener_socket( TCP );
        listener_socket.bind( Address( "0.0.0.0", 5555 ) );
        listener_socket.listen();

        Poller poll_for_clients;

        EventLoop pageload_loop;

        /* listen for new clients */
        poll_for_clients.add_action( Poller::Action( listener_socket, Direction::In,
                                                     [&] () {
                                                         Socket client = listener_socket.accept();
                                                         pageload_loop.add_child_process( "new_client", [&] () {
                                                             handle_client( client, veth_counter );
                                                             return EXIT_SUCCESS;
                                                         } );
                                                         veth_counter++;
                                                         return ResultType::Continue;
                                                     },
                                                     [&] () { return true; } ) );

        while ( true ) {
            if ( poll_for_clients.poll( -1 ).result == Poller::Result::Type::Exit ) {
                return EXIT_SUCCESS;
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
