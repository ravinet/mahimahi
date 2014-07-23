/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <unistd.h>

#include "util.hh"
#include "exception.hh"
#include "socket.hh"
#include "address.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " server_ip_address url(eg. www.mit.edu)" );
        }

        Socket server( TCP );
        server.connect( Address( string( argv[ 1 ] ), 5555 ) );

        string request = "GET / HTTP/1.1\r\nHost: " + string( argv[ 2 ] ) + "\r\n\r\n";

        server.write( request );

        sleep(100);
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
