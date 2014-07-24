/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "socket.hh"
#include "event_loop.hh"

using namespace std;
using namespace PollerShortNames;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 4 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " host port url" );
        }

        Socket server( TCP );
        server.connect( Address( argv[ 1 ], argv[ 2 ] ) );

        server.write( "GET / HTTP/1.1\r\nHost: " + string( argv[ 3 ] ) + "\r\n\r\n" );

        while ( not server.eof() ) {
            cout << server.read() << endl;
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
