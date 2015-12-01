/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "trivial_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "tunnelclient.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 4 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " IP PORT PRIVATE-IP [command...]" );
        }

	const Address server{ argv[ 1 ], argv[ 2 ] };
	const Address private_address { argv[ 3 ], "0" };

        vector< string > command;

        if ( argc == 4 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 4; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        TunnelClient<TrivialQueue> tunnelled_app( user_environment, server, private_address );

        tunnelled_app.start_uplink( "[tunnel " + server.str() + "] ",
				    command, 57  );
        return tunnelled_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
