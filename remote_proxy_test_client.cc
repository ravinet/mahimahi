/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>

#include "util.hh"
#include "exception.hh"
#include "socket.hh"
#include "address.hh"
#include "poller.hh"

using namespace std;
using namespace PollerShortNames;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " server_ip_address url(eg. www.mit.edu)" );
        }

        Socket server( TCP );
        server.connect( Address( string( argv[ 1 ] ), 5555 ) );

        string request = "GET / HTTP/1.1\r\nHost: " + string( argv[ 2 ] ) + "\r\n\r\n";

        Poller poller;

        bool sent_request = false;

        /* read messages from server and print to screen */
        poller.add_action( Poller::Action( server, Direction::In,
                                           [&] () {
                                               string message = server.read();
                                               cout << message << endl;
                                               return ResultType::Continue;
                                           },
                                           [&] () { return true; } ) );

        /* send request to server */
        poller.add_action( Poller::Action( server, Direction::Out,
                                           [&] () {
                                               server.write( request );
                                               sent_request = true;
                                               return ResultType::Continue;
                                           },
                                           [&] () { return not sent_request; } ) );
        while ( true ) {
            if ( poller.poll( -1 ).result == Poller::Result::Type::Exit ) {
                return EXIT_FAILURE;
            }
            if ( server.eof() ) { /* Exit because server ended connection */
                cout << "EOF" << endl;
                return EXIT_SUCCESS;
            }
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
