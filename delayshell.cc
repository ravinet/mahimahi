/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "delay_queue.hh"

#include "packetshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 3 ) {
            throw Exception( "Usage", string( argv[ 0 ] ) + " propagation-delay [in milliseconds] command_to_run" );
        }

        vector< string > program_to_run;
        for ( int num_args = 2; num_args < argc; num_args++ ) {
            program_to_run.emplace_back( string( argv[ num_args ] ) );
        }

        const uint64_t delay_ms = myatoi( argv[ 1 ] );

        PacketShell<DelayQueue> delay_shell_app( "delay" );

        delay_shell_app.start_uplink( "[delay " + to_string( delay_ms ) + " ms] ", program_to_run, user_environment, delay_ms );
        delay_shell_app.start_downlink( delay_ms );
        return delay_shell_app.wait_for_exit();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
