/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>

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

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 1 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) );
        }

        Socket listener_socket( TCP );
        listener_socket.bind( Address( "0.0.0.0", 5555 ) );
        listener_socket.listen();

        Socket client = listener_socket.accept();

        Poller poller;

        HTTPRequestParser request_parser;

        /* completed requests from client are serialized and sent to server */
        poller.add_action( Poller::Action( client, Direction::In,
                                           [&] () {
                                               string buffer = client.read();
                                               request_parser.parse( buffer );
                                               return ResultType::Continue;
                                           },
                                           [&] () { return true; } ) );

        EventLoop pageload_loop;

        while ( true ) {
            if ( not request_parser.empty() )  { /* we have a complete request */
                string url;
                HTTPRequest incoming_request = request_parser.front();
                cout << incoming_request.str();
                request_parser.pop();
                if ( incoming_request.has_header( "Host" ) ) {
                    url = incoming_request.get_header_value( "Host" );
                }
                pageload_loop.add_child_process( "url", [&] () {
                    URLLoader load_page;
                    load_page.get_all_resources( url );
                    return EXIT_SUCCESS;
                } );
            }
            if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
                return EXIT_SUCCESS;
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
