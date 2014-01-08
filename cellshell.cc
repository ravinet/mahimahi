/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "cell_queue.hh"

#include "packetshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " uplink downlink" );
        }

        const std::string uplink_filename = argv[ 1 ];
        const std::string downlink_filename = argv[ 2 ];

        PacketShell<CellQueue> cell_shell_app( "cell" );

        cell_shell_app.start_uplink( "[cell, up=" + uplink_filename + ", down=" + downlink_filename + "] ",
                                     user_environment, uplink_filename );
        cell_shell_app.start_downlink( downlink_filename );
        return cell_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
