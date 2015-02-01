/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "loss_queue.hh"
#include "util.hh"
#include "packetshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 2 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " loss-percentage [command...]" );
        }

        const uint64_t loss_percentage = myatoi( argv[ 1 ] );

        vector< string > command;

        if ( argc == 2 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        PacketShell<LossQueue> loss_shell_app( "loss" );

        loss_shell_app.start_uplink( "[loss " + to_string( loss_percentage ) + "%] ",
                                      user_environment,
                                      command,
                                      loss_percentage );
        loss_shell_app.start_downlink( loss_percentage );
        return loss_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
