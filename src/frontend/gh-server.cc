/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "trivial_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "tunnelserver.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 1 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) );
        }

        TunnelServer<TrivialQueue> tunnelled_app( "tunnel", user_environment );
        tunnelled_app.start_downlink( 100 );

        return tunnelled_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
